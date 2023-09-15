# C-Shell
*A CLI shell program*

## What does C-Shell do?
As a shell, this program allows the user to execute commands on a Unix based OS.

Supported commands include:
* all standard unix shell commands (ls, cd, whoami, echo, etc.)
* prev: repeats the previous command
* source -sourcefile: executes the contents of sourcefile as input

Supported features include:
* Input redirection with <
* Output redirection with >
* Piping with |
* Multiple commands in a line with ;

## How to run the project?

1. Clone the repository onto a machine running linux
2. Run the makefile with 'make' in your terminal to create the executable
3. ./shell to run the program
