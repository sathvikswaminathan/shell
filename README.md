# shell
Implementation of a very basic shell

## Functionality

- Built-in commands
    - `exit`: ends shell session
    - `cd [path]` : changes directory
    - `history`: displays history of commands
    - `kill [pid]`: terminates process
    - `jobs`: lists currently running jobs
    - `help`: Displays all commands
    
-  Run jobs in foreground or background(&)


## Build and Run

`gcc shell.c -lreadline -o shell`

`./shell`
