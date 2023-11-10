#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "jobs.h"

// Global variable to allow for a change in input count to shell
size_t count = 1024;
// Global job list for managing job implementation
job_list_t *job_list;

/*
     * This function prints errors from parse to fprintf and null resets arrays
     to prevent segfaults and memory leaks
     *
     * msg - the error message to be printed to standard error
     * tokens - the array of tokens to be reset to null.
     * argv - arguments array to be reset to null for the next shell prompt
     *
     */

void error_reset_handler(char *msg, char *tokens[count / 2],
                         char *argv[count / 2])
{
    fprintf(stderr, "%s\n", msg);
    memset(tokens, '\0', count / 2 * sizeof(char *));
    memset(argv, '\0', count / 2 * sizeof(char *));
}

/*
This function parses input from the buffer and fills out an array of tokens,
an array of arguments and tracks appropriate input output redirections.
buffer- a buffer array that contains the input populated by read
tokens - an null array to be populated by parsed tokens
argv- a null array to be populated by arguments
argc - an argument counter used to determine if built in functions have proper
syntax output_append_path- used to store the file path of ">>" redirection
output_redirect_path- used to store the file path of ">" redirection
input_redirect_path - used to store the file path of "<" redirection
*/
void parse(char buffer[count], char *tokens[count / 2], char *argv[count / 2],
           int *argc, char **output_append_path, char **output_redirect_path,
           char **input_redirect_path, int *background_flag)
{
    // Counts instances of output redirection
    int output_redirect_count = 0;
    // Used to track what type of redirection the previous token was
    int saw_rdr_flag = 0;
    // Counts instances of input redirection
    int input_redirect_count = 0;
    int index = 0;
    // Get the first token
    char *token_pointer = strtok(buffer, " \n\t");
    // store a previous token to check
    char *prev_token;
    // Loop while input exists
    while (token_pointer != NULL)
    {
        // Flags wether the current token is the corresponding redirection
        // symbol
        int output_redirect_flag = !strcmp(token_pointer, ">");
        int input_redirect_flag = !strcmp(token_pointer, "<");
        int output_append_flag = !strcmp(token_pointer, ">>");

        if (saw_rdr_flag != 0)
        {
            // If the previous instance and current instance are both
            // redirection symbols through an error
            if (output_append_flag || output_redirect_flag)
            {
                error_reset_handler(
                    "syntax error: output file is a redirection symbol", tokens,
                    argv);
                return;
            }
            else if (input_redirect_flag)
            {
                error_reset_handler(
                    "syntax error: input file is a redirection symbol", tokens,
                    argv);
                return;
            }
            else
            {
                // Set path files appropriately if previous instance was a
                // redirection symbol
                switch (saw_rdr_flag)
                {
                case 1:
                    (*output_redirect_path) = token_pointer;
                    saw_rdr_flag = 0;
                    break;
                case 2:
                    (*input_redirect_path) = token_pointer;
                    saw_rdr_flag = 0;
                    break;
                case 3:
                    (*output_append_path) = token_pointer;
                    saw_rdr_flag = 0;
                    break;
                default:
                    break;
                }
                token_pointer = strtok('\0', " \n\t");
                // Check if the next token is null, (i.e no file path after
                // ouput symbol)
                if ((saw_rdr_flag == 1 || saw_rdr_flag == 3) &&
                    token_pointer == NULL)
                {
                    error_reset_handler("syntax error: no output file", tokens,
                                        argv);
                    return;
                }
                // Check if the next token is null, (i.e no file path after
                // input symbol)
                else if (saw_rdr_flag == 2 && token_pointer == NULL)
                {
                    error_reset_handler("syntax error: no input file", tokens,
                                        argv);
                    return;
                }
                saw_rdr_flag = 0;
                continue;
            }
        }
        // Count redirect symbols in input line, if more than one exists print
        // error
        if (input_redirect_flag)
        {
            if (input_redirect_count != 0)
            {
                error_reset_handler("syntax error: multiple input files",
                                    tokens, argv);
                return;
            }
            else
            {
                input_redirect_count += 1;
            }
        }
        else if (output_redirect_flag || output_append_flag)
        {
            if (output_redirect_count != 0)
            {
                error_reset_handler("syntax error: mulitple output files",
                                    tokens, argv);
                return;
            }
            else
            {
                output_redirect_count += 1;
            }
        }

        if (output_redirect_flag)
        {
            // Set corresponding flag for next instance
            saw_rdr_flag = 1;
            token_pointer = strtok('\0', " \n\t");
            // Check if the next token is null, (i.e no file path after
            // redirection symbol)
            if (token_pointer == NULL)
            {
                error_reset_handler("syntax error: no output file", tokens,
                                    argv);
                return;
            }
            // Next instantiation of the loop
            continue;
        }
        else if (input_redirect_flag)
        {
            // Set corresponding flag for next instance
            saw_rdr_flag = 2;
            token_pointer = strtok('\0', " \n\t");
            // Check if the next token is null, (i.e no file path after
            // redirection symbol)
            if (token_pointer == NULL)
            {
                error_reset_handler("syntax error: no input file", tokens,
                                    argv);
                return;
            }
            continue;
        }
        else if (output_append_flag)
        {
            // Set corresponding flag for next instance
            saw_rdr_flag = 3;
            token_pointer = strtok('\0', " \n\t");
            // Check if the next token is null, (i.e no file path after
            // redirection symbol)
            if (token_pointer == NULL)
            {
                error_reset_handler("syntax error: no output file", tokens,
                                    argv);
                return;
            }
            continue;
        }
        tokens[index] = token_pointer;
        // Check if index is 0, check if first token is a path and move
        // appropriately
        if (index == 0)
        {
            // In order to set the first ARGV, where the last slash is
            char *last_slash = strrchr(tokens[0], '/');
            if (last_slash != NULL)
            {
                argv[index] = last_slash + 1;
                // Count argument counter each time a token is added to argv
                (*argc)++;
            }
            else
            {
                argv[index] = token_pointer;
                (*argc)++;
            }
        }
        else
        {
            argv[index] = token_pointer;
            (*argc)++;
        }
        prev_token = token_pointer;
        token_pointer = strtok('\0', " \n\t");
        index++;
    }
    // Check for background symbol
    if (prev_token != NULL && !strcmp(prev_token, "&"))
    {
        (*background_flag) = 1;
        // Remove & from arrays
        tokens[index - 1] = NULL;
        argv[index - 1] = NULL;
    }
    else
    {
        tokens[index] = NULL;
        argv[index] = NULL;
    }

    // If a redirect path was set, and the argument vectors isn' set through no
    // command erors.
    if ((*output_redirect_path || *input_redirect_path ||
         *output_append_path) &&
        (*argv) == NULL)
    {
        error_reset_handler("error:redirects with no command", tokens, argv);
        return;
    }
}

/* This function handles input output redirection my closing and opening file
descriptors corresponding to wether the input path exists. input_redirect_path -
pointer to the input_redirect_path saved from parse output_redirect_path -
pointer to the output_redirect_path saved from parse output_append - pointer to
the output_append_path saved from parse
*/
void io_redirection(char **input_redirect_path, char **output_redirect_path,
                    char **output_append)
{
    if ((*input_redirect_path))
    {
        if (close(0) == -1)
        {
            perror("close");
            cleanup_job_list(job_list);
            exit(1);
        }
        if (open((*input_redirect_path), O_RDONLY, 0666) == -1)
        {
            perror("open");
            cleanup_job_list(job_list);
            exit(1);
        }
    }
    if ((*output_redirect_path))
    {
        if (close(1) == -1)
        {
            perror("close");
            cleanup_job_list(job_list);
            exit(1);
        }
        if (open((*output_redirect_path), O_CREAT | O_TRUNC | O_WRONLY, 0666) ==
            -1)
        {
            perror("open");
            cleanup_job_list(job_list);
            exit(1);
        }
    }
    else if ((*output_append))
    {
        if (close(1) == -1)
        {
            perror("close");
            cleanup_job_list(job_list);
            exit(1);
        }
        if (open((*output_append), O_CREAT | O_APPEND | O_WRONLY, 0666) == -1)
        {
            perror("open");
            cleanup_job_list(job_list);
            exit(1);
        }
    }
}

// Executes built in cd command by calling chdir
// argv- input argument vector
// argc - pointer to argument counter
void cd(char *argv[count / 2], int *argc)
{
    if ((*argc) != 2)
    {
        fprintf(stderr, "Syntax error with cd");
    }
    else if (chdir(argv[1]) != 0)
    {
        perror("chdir");
    }
}
// Executes built in ln by calling link
// argv- input argument vector
// argc - pointer to argument counter
void ln(char *argv[count / 2], int *argc)
{
    if ((*argc) != 3)
    {
        fprintf(stderr, "Syntax error with ln");
    }
    else if (link(argv[1], argv[2]) != 0)
    {
        perror("link");
    }
}
// Executes built in rm function by calling unlink
// argv- input argument vector
// argc - pointer to argument counter
void rm(char *argv[count / 2], int *argc)
{
    if ((*argc) != 2)
    {
        fprintf(stderr, "Syntax error with rm");
    }
    else if (unlink(argv[1]) != 0)
    {
        perror("unlink");
    }
}
// Executes built in jobs function by calling provided jobs function
// argc - pointer to argument counter
void jobs_builtin(int *argc)
{
    if ((*argc) != 1)
    {
        fprintf(stderr, "Syntax error with jobs");
    }
    else
    {
        jobs(job_list);
    }
}
// Executed built in bg function by sending kill to all processes that share the
// job id and updating the job list.
// argv- input argument vector
// argc - pointer to argument counter
void bg(char *argv[count / 2], int *argc)
{
    if ((*argc) == 2 && argv[0][1] != '\0' && argv[1][0] == '%')
    {
        // Store job id for job list
        int jid = atoi(&argv[1][1]);
        int pid = get_job_pid(job_list, jid);
        if (pid == -1)
        {
            fprintf(stderr, "job not found\n");
        }
        else
        {
            // Use -pid so it sends to all processes that have pid as a process
            // group id.
            if (kill(-pid, SIGCONT) == -1)
            {
                perror("kill");
            }
            update_job_jid(job_list, jid, RUNNING);
        }
    }
    else
    {
        fprintf(stderr, "Incorrect Syntax for bg builtin");
    }
}
// Executed built in fg function by sending SIGCONT to the job, placing in the
// foreground and then reaping properly
// argv- input argument vector
// argc - pointer to argument counter
void fg(char *argv[count / 2], int *argc)
{
    int status;
    if ((*argc) == 2 && argv[0][1] != '\0' && argv[1][0] == '%')
    {
        // Store job id for job list
        int jid = atoi(&argv[1][1]);
        int pid = get_job_pid(job_list, jid);
        if (pid == -1)
        {
            fprintf(stderr, "job not found \n");
        }
        else
        {
            // Give the foreground job terminal control
            if (tcsetpgrp(0, pid) == -1)
            {
                perror("tcsetpgrp");
                // Cleanup jobs list before each exit
                cleanup_job_list(job_list);
                exit(1);
            }
            // Send SIGCONT to all processes that have pid as a process group
            // id.
            if (kill(-pid, SIGCONT) == -1)
            {
                perror("kill");
            }

            // Remove job from the job list
            if (remove_job_pid(job_list, pid) == -1)
            {
                fprintf(stderr, "error removing job");
            }

            // Reap the process (wait for status change)
            waitpid(pid, &status, WUNTRACED);

            // Handle status changes
            if (WIFSIGNALED(status))
            {
                printf("(%d) terminated by signal %d", pid, WTERMSIG(status));
            }
            else if (WIFSTOPPED(status))
            {
                // Update job status to stopped
                if (add_job(job_list, jid, pid, STOPPED, argv[0]) == -1)
                {
                    fprintf(stderr, "Updating Job after stopped error");
                }
                else
                {
                    // update job did not error so print the exit message
                    printf("[%d] (%d) suspended by signal %d\n", jid, pid,
                           WSTOPSIG(status));
                }
            }
            // Return control to shell
            if (tcsetpgrp(0, getpgrp()) == -1)
            {
                perror("tcsetpgrp");
                // Cleanup jobs list before each exit
                cleanup_job_list(job_list);
                exit(1);
            }
        }
    }
    else
    {
        fprintf(stderr, "fg syntax error");
    }
}
// Reaps and handles status changes for foreground processes
// fg_pid - process id of the foreground process
// jid - job id
// command - a char * to the name of the command given to the shell
void post_foreground_handler(pid_t fg_pid, int *jid, char *command)
{
    // Create a status integer for waitpid to put info into
    int status;
    // Reap foreground process
    waitpid(fg_pid, &status, WUNTRACED);
    // Return terminal control to shell
    if (tcsetpgrp(0, getpgrp()) == -1)
    {
        perror("tcsetpgrp");
        // Cleanup jobs list before each exit
        cleanup_job_list(job_list);
        exit(1);
    }
    // Print statement when terminated by signal
    if (WIFSIGNALED(status))
    {
        if (printf("(%d) terminated by signal %d\n", fg_pid,
                   WTERMSIG(status)) == -1)
        {
            perror("printf");
        }
    }
    // Whenever a job (foreground OR background) is stopped, it should be
    // added to the job list if it has not been already, and its state
    // updated.
    else if (WIFSTOPPED(status))
    {
        if (add_job(job_list, (*jid), fg_pid, STOPPED, command) == -1)
        {
            fprintf(stderr, "add  stopped job in foreground error");
        }
        if (printf("[%d] (%d) suspended by signal %d\n", (*jid), fg_pid,
                   WSTOPSIG(status)) == -1)
        {
            perror("printf");
        }
        (*jid)++;
    }
}
// This function reaps and handles status changes for all processes
void process_handler()
{
    // Create status integer for waitpid to input info into
    int status;
    // Create a variable to hold pid on success, returns the process ID of
    // the child whose state has changed;
    int pid;
    // Use negative one to wait for for any child process whose process
    // group ID is equal to the absolute value of pid. (which should work
    // because we setpgid
    //  to be the same as unique pid)
    while ((pid = waitpid(-1, &status, WNOHANG | WCONTINUED | WUNTRACED)) > 0)
    {
        // Get the job id to handle calls to job functions
        int jid = get_job_jid(job_list, pid);

        // If job isn't in the job list move to next pid
        if (jid == -1)
        {
            continue;
        }
        // Check termination cases according to order on handout
        if (WIFEXITED(status))
        {
            if (remove_job_pid(job_list, pid) == -1)
            {
                fprintf(stderr, "Removing Job after exit error");
            }
            else
            {
                // remove job did not error so print the exit message
                printf("[%d] (%d) terminated with exit status %d\n", jid, pid,
                       WEXITSTATUS(status));
            }
        }
        else if (WIFSIGNALED(status))
        {
            if (remove_job_pid(job_list, pid) == -1)
            {
                fprintf(stderr, "Removing Job after signal interuption error");
            }
            else
            {
                // remove job did not error so print the exit message
                printf("[%d] (%d) terminated by signal %d", jid, pid,
                       WTERMSIG(status));
            }
        }
        else if (WIFSTOPPED(status))
        {
            // Update job status to stopped
            if (update_job_jid(job_list, jid, STOPPED) == -1)
            {
                fprintf(stderr, "Updating Job after stopped error");
            }
            else
            {
                // update job did not error so print the exit message
                printf("[%d] (%d) suspended by signal %d\n", jid, pid,
                       WSTOPSIG(status));
            }
        }
        else if (WIFCONTINUED(status))
        {
            // Update job status to stopped
            if (update_job_jid(job_list, jid, RUNNING) == -1)
            {
                fprintf(stderr, "Updating Job after resumed error");
            }
            else
            {
                // update job did not error so print the exit message
                printf("[%d] (%d) resumed\n", jid, pid);
            }
        }
    }
}

int main()
{
    char buffer[count];
    char *tokens[count / 2];
    char *argv[count / 2];
    ssize_t input_bytes_read;
    // Create an argument counter to track potential syntax erors
    int argc = 0;
    // Create a variable to track output redirection with append
    char *output_append_path = NULL;
    // Create a variable to track output redirection file path
    char *output_redirect_path = NULL;
    // Create a variable to track input redirect file path
    char *input_redirect_path = NULL;
    // Create a flag to track wether a process is input for background
    int background_flag = 0;
    // Create the jobs list
    job_list = init_job_list();
    // Job Id
    int jid = 1;
    // Ignore the following Signals by default
    // Restore the following Signals to default
    if (signal(SIGINT, SIG_IGN) == SIG_ERR)
    {
        perror("signal");
        cleanup_job_list(job_list);
        exit(1);
    }
    if (signal(SIGTSTP, SIG_IGN) == SIG_ERR)
    {
        perror("signal");
        cleanup_job_list(job_list);
        exit(1);
    }
    if (signal(SIGTTOU, SIG_IGN) == SIG_ERR)
    {
        perror("signal");
        cleanup_job_list(job_list);
        exit(1);
    }

#ifdef PROMPT
    if (printf("33sh> ") < 0)
    {
        fprintf(stderr, "Error printing prompt to terminal");
    }
    if (fflush(stdout) < 0)
    {
        fprintf(stderr, "Error flushing printing terminal prompt");
    }
#endif
    // Ensure arrays are all null.
    memset(buffer, '\0', count);
    memset(tokens, '\0', count / 2 * sizeof(char *));
    memset(argv, '\0', count / 2 * sizeof(char *));
    while ((input_bytes_read = read(0, buffer, count)) != 0)
    {
        // If no input is read return and reprompt
        if (input_bytes_read == 0)
        {
            printf("\n");
            // Clean before every return
            cleanup_job_list(job_list);
            return 0;
        }
        // check for read error
        else if (input_bytes_read == -1)
        {
            perror("read");
        }
        // Set end of the read input to Null
        buffer[input_bytes_read] = '\0';
        parse(buffer, tokens, argv, &argc, &output_append_path,
              &output_redirect_path, &input_redirect_path, &background_flag);

        char *built_in = tokens[0];

        if (built_in != NULL)
        {
            // Check if the first token matches built ins and handle
            // appropriately
            if (strcmp(built_in, "exit") == 0)
            {
                // Clean Job list before every return
                cleanup_job_list(job_list);
                return 0;
            }
            else if (strcmp(built_in, "cd") == 0)
            {
                cd(argv, &argc);
            }
            else if (strcmp(built_in, "ln") == 0)
            {
                ln(argv, &argc);
            }
            else if (strcmp(built_in, "rm") == 0)
            {
                rm(argv, &argc);
            }
            else if (strcmp(built_in, "jobs") == 0)
            {
                jobs_builtin(&argc);
            }
            else if (strcmp(built_in, "bg") == 0)
            {
                bg(argv, &argc);
            }
            else if (strcmp(built_in, "fg") == 0)
            {
                fg(argv, &argc);
            }
            else
            {
                // Execute child process
                pid_t child_pid = fork();
                if (child_pid == -1)
                {
                    perror("fork");
                    cleanup_job_list(job_list);
                    exit(1);
                }
                if (child_pid == 0)
                {
                    // Find the unique Process id
                    pid_t current_pid = getpid();
                    if (setpgid(current_pid, current_pid) == -1)
                    {
                        perror("setpgid");
                        exit(1);
                    }
                    // If the process is not running in the background, set
                    // the controlling terminal
                    if (!background_flag)
                    {
                        if (tcsetpgrp(0, getpgrp()) == -1)
                        {
                            perror("tcsetpgrp");
                            exit(1);
                        }
                    }

                    // Restore the following Signals to default
                    if (signal(SIGINT, SIG_DFL) == SIG_ERR)
                    {
                        perror("signal");
                        cleanup_job_list(job_list);
                        exit(1);
                    }
                    if (signal(SIGTSTP, SIG_DFL) == SIG_ERR)
                    {
                        perror("signal");
                        cleanup_job_list(job_list);
                        exit(1);
                    }
                    if (signal(SIGTTOU, SIG_DFL) == SIG_ERR)
                    {
                        perror("signal");
                        cleanup_job_list(job_list);
                        exit(1);
                    }

                    io_redirection(&input_redirect_path, &output_redirect_path,
                                   &output_append_path);
                    execv(tokens[0], argv);
                    perror("execv");

                    exit(1);
                }
                else
                {
                    if (background_flag)
                    {
                        if (add_job(job_list, jid, child_pid, RUNNING,
                                    built_in) == -1)
                        {
                            fprintf(stderr, "add background job error");
                        }
                        if (printf("[%d] (%d)\n", jid, child_pid) < 0)
                        {
                            perror("printf");
                        }
                        jid++;
                    }
                    else
                    {
                        // Abstract Out Foreground Process Handler
                        post_foreground_handler(child_pid, &jid, built_in);
                    }
                    background_flag = 0;
                }
            }
        }
        process_handler();
        argc = 0;
        // Create a variable to track output redirection with append
        output_append_path = NULL;
        // Create a variable to track output redirection file path
        output_redirect_path = NULL;
        // Create a variable to track input redirect file path
        input_redirect_path = NULL;

#ifdef PROMPT
        if (printf("33sh> ") < 0)
        {
            fprintf(stderr, "Error printing prompt to terminal");
        }
        if (fflush(stdout) < 0)
        {
            fprintf(stderr, "Error flushing printing terminal prompt");
        }
#endif
        // Reset all arrays
        memset(buffer, '\0', count);
        memset(tokens, '\0', count / 2 * sizeof(char *));
        memset(argv, '\0', count / 2 * sizeof(char *));
    }
    // Continue to clean job list before every return
    cleanup_job_list(job_list);
    return 0;
}
