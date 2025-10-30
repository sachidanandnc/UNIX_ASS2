# UNIX_ASS2

# 1. Install WSL (run in PowerShell as Administrator)
wsl --install

# 2. Restart your computer

# 3. Open "Ubuntu" from Start Menu

# 4. Inside Ubuntu terminal:
cd ~
mkdir project
cd project

# 5. Create your file
nano shell.c
# (paste code, Ctrl+X, Y, Enter)

# 6. Install readline
sudo apt-get update
sudo apt-get install libreadline-dev gcc

# 7. Compile and run
gcc -o myshell shell.c -lreadline
./myshell

note: download necessary libraries  
   - gcc-core
   - make
   - libreadline-devel
