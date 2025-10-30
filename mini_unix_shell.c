// Mini Unix Shell - Enhanced Version for Project Submission
// Team Project: 5 Students
// Compile: gcc -o myshell shell.c -lreadline
// Run: ./myshell

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <errno.h>
#include <time.h>

#define MAX_ARGS 64
#define MAX_PIPES 10
#define MAX_JOBS 64
#define HISTORY_FILE ".myshell_history"
#define VERSION "1.0"

// ============= COLOR CODES =============
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_WHITE   "\033[1;37m"

// ============= DATA STRUCTURES =============

typedef struct {
    char *cmd;
    char **args;
    char *input_file;
    char *output_file;
    int append;
} Command;

typedef struct {
    Command *commands;
    int num_commands;
    int background;
} Pipeline;

typedef struct {
    pid_t pid;
    char *command;
    int running;
    time_t start_time;
} Job;

// ============= GLOBAL VARIABLES =============

Job jobs[MAX_JOBS];
int job_count = 0;
int command_count = 0;
int show_stats = 0;

// ============= SIGNAL HANDLERS =============

void sigchld_handler(int sig) {
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < job_count; i++) {
            if (jobs[i].pid == pid && jobs[i].running) {
                time_t elapsed = time(NULL) - jobs[i].start_time;
                printf("\n%s[Job %d]%s Done (%.0fs)\t%s%s%s\n", 
                    COLOR_GREEN, i + 1, COLOR_RESET,
                    (double)elapsed,
                    COLOR_CYAN, jobs[i].command, COLOR_RESET);
                jobs[i].running = 0;
                free(jobs[i].command);
                jobs[i].command = NULL;
            }
        }
    }
}

void sigint_handler(int sig) {
    printf("\n");
    rl_on_new_line();
    rl_replace_line("", 0);
    rl_redisplay();
}

// ============= UTILITY FUNCTIONS =============

void print_banner() {
    printf("\n");
    printf("%s╔══════════════════════════════════════════════════════════╗%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║           Mini Unix Shell - Version %s               ║%s\n", COLOR_CYAN, VERSION, COLOR_RESET);
    printf("%s╠══════════════════════════════════════════════════════════╣%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║  Team Project: Unix Shell Implementation                ║%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║                                                          ║%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║  Features:                                               ║%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║    • Command Parsing & Execution                         ║%s\n", COLOR_GREEN, COLOR_RESET);
    printf("%s║    • Pipes (|) & Redirection (>, <, >>)                  ║%s\n", COLOR_GREEN, COLOR_RESET);
    printf("%s║    • Background Jobs (&)                                 ║%s\n", COLOR_GREEN, COLOR_RESET);
    printf("%s║    • Built-in Commands (cd, pwd, exit, etc.)             ║%s\n", COLOR_GREEN, COLOR_RESET);
    printf("%s║    • Command History with Arrow Keys                     ║%s\n", COLOR_GREEN, COLOR_RESET);
    printf("%s║                                                          ║%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║  Type 'help' for available commands                      ║%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("%s║  Type 'exit' to quit                                     ║%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("%s╚══════════════════════════════════════════════════════════╝%s\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");
}

void print_help() {
    printf("\n%s=== Built-in Commands ===%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%scd [dir]%s       - Change directory\n", COLOR_GREEN, COLOR_RESET);
    printf("%spwd%s            - Print working directory\n", COLOR_GREEN, COLOR_RESET);
    printf("%sexit%s           - Exit the shell\n", COLOR_GREEN, COLOR_RESET);
    printf("%shistory%s        - Show command history\n", COLOR_GREEN, COLOR_RESET);
    printf("%sjobs%s           - List background jobs\n", COLOR_GREEN, COLOR_RESET);
    printf("%shelp%s           - Show this help message\n", COLOR_GREEN, COLOR_RESET);
    printf("%sclear%s          - Clear the screen\n", COLOR_GREEN, COLOR_RESET);
    printf("%sstats%s          - Toggle command statistics\n", COLOR_GREEN, COLOR_RESET);
    
    printf("\n%s=== Special Characters ===%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s|%s              - Pipe (connect commands)\n", COLOR_YELLOW, COLOR_RESET);
    printf("%s>%s              - Output redirection\n", COLOR_YELLOW, COLOR_RESET);
    printf("%s>>%s             - Append output\n", COLOR_YELLOW, COLOR_RESET);
    printf("%s<%s              - Input redirection\n", COLOR_YELLOW, COLOR_RESET);
    printf("%s&%s              - Run in background\n", COLOR_YELLOW, COLOR_RESET);
    
    printf("\n%s=== Examples ===%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  ls -la | grep txt\n");
    printf("  echo \"hello\" > file.txt\n");
    printf("  sort < input.txt > output.txt\n");
    printf("  sleep 10 &\n");
    printf("  cat file1.txt | grep error | wc -l\n\n");
}

// ============= COMMAND PARSING (Student 1) =============

void free_command(Command *cmd) {
    if (cmd->cmd) free(cmd->cmd);
    if (cmd->args) {
        for (int i = 0; cmd->args[i]; i++) free(cmd->args[i]);
        free(cmd->args);
    }
    if (cmd->input_file) free(cmd->input_file);
    if (cmd->output_file) free(cmd->output_file);
}

void free_pipeline(Pipeline *pl) {
    if (pl->commands) {
        for (int i = 0; i < pl->num_commands; i++) {
            free_command(&pl->commands[i]);
        }
        free(pl->commands);
    }
}

Pipeline parse_input(char *input) {
    Pipeline pl = {NULL, 0, 0};
    
    if (!input || strlen(input) == 0) return pl;
    
    // Remove leading/trailing whitespace
    while (*input == ' ' || *input == '\t') input++;
    
    int len = strlen(input);
    while (len > 0 && (input[len-1] == ' ' || input[len-1] == '\t' || input[len-1] == '\n')) {
        input[--len] = '\0';
    }
    
    if (len == 0) return pl;
    
    // Check for background
    if (input[len - 1] == '&') {
        pl.background = 1;
        input[len - 1] = '\0';
        len--;
        while (len > 0 && input[len-1] == ' ') input[--len] = '\0';
    }
    
    // Split by pipes
    char *pipe_tokens[MAX_PIPES];
    int pipe_count = 0;
    char *token = strtok(input, "|");
    
    while (token && pipe_count < MAX_PIPES) {
        pipe_tokens[pipe_count++] = token;
        token = strtok(NULL, "|");
    }
    
    pl.num_commands = pipe_count;
    pl.commands = malloc(pipe_count * sizeof(Command));
    
    // Parse each command
    for (int i = 0; i < pipe_count; i++) {
        Command *cmd = &pl.commands[i];
        cmd->input_file = NULL;
        cmd->output_file = NULL;
        cmd->append = 0;
        
        char *args[MAX_ARGS];
        int arg_count = 0;
        
        char *str = pipe_tokens[i];
        char *word = strtok(str, " \t\n");
        
        while (word && arg_count < MAX_ARGS - 1) {
            if (strcmp(word, ">") == 0) {
                word = strtok(NULL, " \t\n");
                if (word) cmd->output_file = strdup(word);
            } else if (strcmp(word, ">>") == 0) {
                word = strtok(NULL, " \t\n");
                if (word) {
                    cmd->output_file = strdup(word);
                    cmd->append = 1;
                }
            } else if (strcmp(word, "<") == 0) {
                word = strtok(NULL, " \t\n");
                if (word) cmd->input_file = strdup(word);
            } else {
                args[arg_count++] = strdup(word);
            }
            word = strtok(NULL, " \t\n");
        }
        args[arg_count] = NULL;
        
        if (arg_count > 0) {
            cmd->cmd = strdup(args[0]);
            cmd->args = malloc((arg_count + 1) * sizeof(char *));
            for (int j = 0; j <= arg_count; j++) {
                cmd->args[j] = args[j];
            }
        } else {
            cmd->cmd = NULL;
            cmd->args = NULL;
        }
    }
    
    return pl;
}

// ============= BUILT-IN COMMANDS (Student 4) =============

int is_builtin(char *cmd) {
    return strcmp(cmd, "cd") == 0 || 
           strcmp(cmd, "pwd") == 0 || 
           strcmp(cmd, "exit") == 0 ||
           strcmp(cmd, "history") == 0 ||
           strcmp(cmd, "jobs") == 0 ||
           strcmp(cmd, "help") == 0 ||
           strcmp(cmd, "clear") == 0 ||
           strcmp(cmd, "stats") == 0;
}

int execute_builtin(Command *cmd) {
    if (strcmp(cmd->cmd, "cd") == 0) {
        char *path = cmd->args[1] ? cmd->args[1] : getenv("HOME");
        if (chdir(path) != 0) {
            printf("%scd: %s%s\n", COLOR_RED, strerror(errno), COLOR_RESET);
        }
        return 1;
    }
    
    if (strcmp(cmd->cmd, "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd))) {
            printf("%s%s%s\n", COLOR_BLUE, cwd, COLOR_RESET);
        } else {
            perror("pwd");
        }
        return 1;
    }
    
    if (strcmp(cmd->cmd, "exit") == 0) {
        printf("\n%s╔════════════════════════════════════╗%s\n", COLOR_CYAN, COLOR_RESET);
        printf("%s║     Thanks for using MyShell!      ║%s\n", COLOR_CYAN, COLOR_RESET);
        printf("%s║     Total commands: %-6d         ║%s\n", COLOR_CYAN, command_count, COLOR_RESET);
        printf("%s╚════════════════════════════════════╝%s\n\n", COLOR_CYAN, COLOR_RESET);
        exit(0);
    }
    
    if (strcmp(cmd->cmd, "history") == 0) {
        HIST_ENTRY **hist_list = history_list();
        if (hist_list) {
            printf("\n%s=== Command History ===%s\n", COLOR_CYAN, COLOR_RESET);
            for (int i = 0; hist_list[i]; i++) {
                printf("%s%5d%s  %s\n", COLOR_YELLOW, i + 1, COLOR_RESET, hist_list[i]->line);
            }
            printf("\n");
        }
        return 1;
    }
    
    if (strcmp(cmd->cmd, "jobs") == 0) {
        int active = 0;
        printf("\n%s=== Background Jobs ===%s\n", COLOR_CYAN, COLOR_RESET);
        for (int i = 0; i < job_count; i++) {
            if (jobs[i].running) {
                time_t elapsed = time(NULL) - jobs[i].start_time;
                printf("%s[%d]%s Running (%.0fs)\t%s%s%s\n", 
                    COLOR_GREEN, i + 1, COLOR_RESET,
                    (double)elapsed,
                    COLOR_YELLOW, jobs[i].command, COLOR_RESET);
                active++;
            }
        }
        if (active == 0) {
            printf("%sNo active background jobs%s\n", COLOR_YELLOW, COLOR_RESET);
        }
        printf("\n");
        return 1;
    }
    
    if (strcmp(cmd->cmd, "help") == 0) {
        print_help();
        return 1;
    }
    
    if (strcmp(cmd->cmd, "clear") == 0) {
        printf("\033[H\033[J");
        return 1;
    }
    
    if (strcmp(cmd->cmd, "stats") == 0) {
        show_stats = !show_stats;
        printf("%sCommand statistics %s%s\n", 
            COLOR_YELLOW, 
            show_stats ? "enabled" : "disabled",
            COLOR_RESET);
        return 1;
    }
    
    return 0;
}

// ============= REDIRECTION (Student 3) =============

int setup_redirection(Command *cmd) {
    if (cmd->input_file) {
        int fd = open(cmd->input_file, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "%s%s: %s%s\n", COLOR_RED, cmd->input_file, strerror(errno), COLOR_RESET);
            return -1;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    
    if (cmd->output_file) {
        int flags = O_WRONLY | O_CREAT;
        flags |= cmd->append ? O_APPEND : O_TRUNC;
        int fd = open(cmd->output_file, flags, 0644);
        if (fd < 0) {
            fprintf(stderr, "%s%s: %s%s\n", COLOR_RED, cmd->output_file, strerror(errno), COLOR_RESET);
            return -1;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    
    return 0;
}

// ============= PROCESS MANAGEMENT & PIPES (Students 2 & 3) =============

void execute_pipeline(Pipeline *pl) {
    if (pl->num_commands == 0 || !pl->commands[0].cmd) return;
    
    // Handle built-in commands (only for single commands, no pipes)
    if (pl->num_commands == 1 && is_builtin(pl->commands[0].cmd)) {
        execute_builtin(&pl->commands[0]);
        return;
    }
    
    int pipes[MAX_PIPES][2];
    pid_t pids[MAX_PIPES];
    
    // Create pipes
    for (int i = 0; i < pl->num_commands - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return;
        }
    }
    
    // Fork and execute each command
    for (int i = 0; i < pl->num_commands; i++) {
        pids[i] = fork();
        
        if (pids[i] < 0) {
            perror("fork");
            return;
        }
        
        if (pids[i] == 0) {  // Child process
            // Set up pipe input
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            
            // Set up pipe output
            if (i < pl->num_commands - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // Close all pipe fds
            for (int j = 0; j < pl->num_commands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Set up file redirection
            if (setup_redirection(&pl->commands[i]) < 0) {
                exit(1);
            }
            
            // Execute command
            execvp(pl->commands[i].cmd, pl->commands[i].args);
            fprintf(stderr, "%s%s: command not found%s\n", COLOR_RED, pl->commands[i].cmd, COLOR_RESET);
            exit(127);
        }
    }
    
    // Parent: close all pipes
    for (int i = 0; i < pl->num_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait or add to job list
    if (pl->background) {
        jobs[job_count].pid = pids[pl->num_commands - 1];
        jobs[job_count].running = 1;
        jobs[job_count].start_time = time(NULL);
        
        // Construct command string
        char cmd_str[1024] = "";
        for (int i = 0; i < pl->num_commands; i++) {
            strcat(cmd_str, pl->commands[i].cmd);
            for (int j = 1; pl->commands[i].args[j]; j++) {
                strcat(cmd_str, " ");
                strcat(cmd_str, pl->commands[i].args[j]);
            }
            if (i < pl->num_commands - 1) strcat(cmd_str, " | ");
        }
        jobs[job_count].command = strdup(cmd_str);
        
        printf("%s[%d] %d%s %s\n", COLOR_GREEN, job_count + 1, pids[pl->num_commands - 1], COLOR_RESET, cmd_str);
        job_count++;
    } else {
        int status;
        clock_t start = clock();
        for (int i = 0; i < pl->num_commands; i++) {
            waitpid(pids[i], &status, 0);
        }
        clock_t end = clock();
        
        if (show_stats) {
            double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
            printf("%s[Execution time: %.3fs]%s\n", COLOR_YELLOW, time_spent, COLOR_RESET);
        }
    }
}

// ============= UI & PROMPT (Student 5) =============

char* get_prompt() {
    static char prompt[512];
    char cwd[256];
    char hostname[64];
    char *user = getenv("USER");
    
    getcwd(cwd, sizeof(cwd));
    gethostname(hostname, sizeof(hostname));
    
    // Get just the directory name
    char *dir = strrchr(cwd, '/');
    dir = dir ? dir + 1 : cwd;
    if (strlen(dir) == 0) dir = "/";
    
    // Beautiful prompt with icons
    snprintf(prompt, sizeof(prompt), 
        "%s┌─[%s%s@%s%s%s]─[%s📁 %s%s]\n%s└─%s$ %s", 
        COLOR_CYAN,
        COLOR_GREEN, user ? user : "user", hostname, COLOR_CYAN,
        COLOR_CYAN,
        COLOR_BLUE, dir, COLOR_CYAN,
        COLOR_CYAN, COLOR_MAGENTA, COLOR_RESET);
    
    return prompt;
}

void load_history() {
    char *home = getenv("HOME");
    if (home) {
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", home, HISTORY_FILE);
        read_history(path);
    }
}

void save_history() {
    char *home = getenv("HOME");
    if (home) {
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", home, HISTORY_FILE);
        write_history(path);
    }
}

// ============= MAIN SHELL LOOP =============

int main() {
    char *input;
    
    // Set up signal handlers
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigint_handler);
    
    // Load command history
    load_history();
    
    // Print banner
    print_banner();
    
    while (1) {
        input = readline(get_prompt());
        
        if (!input) {  // Ctrl+D
            printf("\n");
            printf("%sUse 'exit' command to quit%s\n", COLOR_YELLOW, COLOR_RESET);
            continue;
        }
        
        if (strlen(input) > 0) {
            add_history(input);
            command_count++;
            
            Pipeline pl = parse_input(input);
            execute_pipeline(&pl);
            free_pipeline(&pl);
        }
        
        free(input);
    }
    
    save_history();
    return 0;
}
