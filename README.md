##  Environment Setup (WSL Installation)

Follow these steps to ensure your Linux environment is ready for development.

### 1\. Install WSL and Ubuntu

Open **PowerShell as Administrator** and run the following command:

```bash
wsl --install
```

### 2\. Restart Your Computer

A system **restart** is required to complete the WSL installation.

### 3\. Open Ubuntu

Launch the **"Ubuntu"** application from the Windows Start Menu.

### 4\. Create Project Directory

Once the Ubuntu terminal is open, navigate to your home directory and create your project folder:

```bash
cd ~
mkdir project
cd project
```

-----

##  Code and Dependency Installation

### 5\. Create the Source File

Use the `nano` text editor to create your C source file:

```bash
nano shell.c
```

**(Paste your C code into the editor. Save and exit by pressing `Ctrl+X`, then `Y`, then `Enter`.)**

### 6\. Install Libraries and Compiler

You need to update your package list and install the C compiler (`gcc`) and the development files for the **Readline library** (`libreadline-dev`).

```bash
sudo apt-get update
sudo apt-get install libreadline-dev gcc
```

> **Required Components Check:**
>
>   *  **`gcc-core`**: The C Compiler tool.
>   *  **`libreadline-devel`** (`libreadline-dev` on Ubuntu): The Library (development files).
>   * *(**`make`** is typically installed automatically as a dependency or is already present, but is good to verify for larger projects.)*

-----

##  Compile and Run

### 7\. Compile the Program

Compile the `shell.c` file, linking it with the Readline library (`-lreadline`), and create the executable named `myshell`.

```bash
gcc -o myshell shell.c -lreadline
```

### 8\. Execute Your Shell

Run your newly compiled program from the terminal:

```bash
./myshell
```

Your custom shell is now running and awaiting commands\!

-----

Would you like me to help you outline the key functions a basic shell written in C needs (like parsing, forking, and executing)?
