# UNIX_ASS2

1. Install WSL (run in PowerShell as Administrator)
   
wsl --install

3. Restart your computer

4. Open "Ubuntu" from Start Menu

5. Inside Ubuntu terminal:
6.    
cd ~
mkdir project
cd project

7. Create your file
   
nano shell.c
(paste code, Ctrl+X, Y, Enter)

7. Install readline
   
sudo apt-get update
sudo apt-get install libreadline-dev gcc

8. Compile and run
   
gcc -o myshell shell.c -lreadline
./myshell

note: make sure u have these  
   - gcc-core -> Compiler tool
   - make -> Build utility
   - libreadline-devel -> Library (development files)
