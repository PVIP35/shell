```
 ____  _          _ _   ____
/ ___|| |__   ___| | | |___ \
\___ \| '_ \ / _ \ | |   __) |
 ___) | | | |  __/ | |  / __/
|____/|_| |_|\___|_|_| |_____|
```

To compile this code run- Make clean all

Bugs I have- This code passes all the tests locally, but seems to do this process
incredibly slowly and times out the autograder, without inefficies apparent to me.

Structure of my program-

My program begins by  the main() function initializes necessary variables, sets up the job list, and handles neccesary signals. It then prompts the user with a shell symbol and reads the user's input. The input undergoes parsing in the parse() function, which separates the command into tokens and handles input/output redirection. Based on the parsed input, the program either executes built-in functions such as cd, ln, rm, and jobs or creates a child process for external commands. The program also handles background processes and job control through functions like bg, fg, and post_foreground_handler. Additionally, the program manages input/output redirection using the io_redirection function. Throughout the execution, the program ensures proper error handling and manages the job list with functions like add_job, remove_job_pid, and update_job_jid. Finally, the process_handler function reaps and handles status changes for all processes.
