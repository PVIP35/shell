#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "jobs.h"

size_t count = 1024;

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
           char **input_redirect_path)
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

        token_pointer = strtok('\0', " \n\t");
        index++;
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
            exit(1);
        }
        if (open((*input_redirect_path), O_RDONLY, 0666) == -1)
        {
            perror("open");
            exit(1);
        }
    }
    if ((*output_redirect_path))
    {
        if (close(1) == -1)
        {
            perror("close");
            exit(1);
        }
        if (open((*output_redirect_path), O_CREAT | O_TRUNC | O_WRONLY, 0666) ==
            -1)
        {
            perror("open");
            exit(1);
        }
    }
    else if ((*output_append))
    {
        if (close(1) == -1)
        {
            perror("close");
            exit(1);
        }
        if (open((*output_append), O_CREAT | O_APPEND | O_WRONLY, 0666) == -1)
        {
            perror("open");
            exit(1);
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
    // int background_flag = 0;
    // Ignore the following Signals by default
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
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
              &output_redirect_path, &input_redirect_path);

        char *built_in = tokens[0];

        if (built_in != NULL)
        {
            // Check if the first token matches built ins and handle
            // appropriately
            if (strcmp(built_in, "exit") == 0)
            {
                return 0;
            }
            else if (strcmp(built_in, "cd") == 0)
            {
                cd(argv, &argc);
            }
            else if (strcmp(tokens[0], "ln") == 0)
            {
                ln(argv, &argc);
            }
            else if (strcmp(tokens[0], "rm") == 0)
            {
                rm(argv, &argc);
            }
            else
            {
                // Execute child process
                pid_t child_pid = fork();
                if (child_pid == -1)
                {
                    perror("fork");
                    exit(1);
                }
                if (child_pid == 0)
                {
                    // Restore the following Signals to default
                    signal(SIGINT, SIG_DFL);
                    signal(SIGTSTP, SIG_DFL);
                    signal(SIGTTOU, SIG_DFL);
                    // Find the unique Process id
                    pid_t current_pid = getpid();
                    if (setpgid(current_pid, current_pid) == -1)
                    {
                        perror("setpgid");
                        exit(1);
                    }
                    // If the process is not running in the background, set the controlling terminal
                    // if (!background_flag && tcsetpgrp(0, getpgrp()) == -1)
                    // {
                    //     perror("tcsetpgrp");
                    //     exit(1);
                    // }
                    io_redirection(&input_redirect_path, &output_redirect_path,
                                   &output_append_path);
                    execv(tokens[0], argv);
                    perror("execv");
                    exit(1);
                }
                else
                {
                    // if (tcsetpgrp(0, getpgrp()) == -1)
                    // {
                    //     perror("tcsetpgrp");
                    //     exit(1);
                    // }
                    wait(0);
                }
            }
        }
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

    return 0;
}
// Executes built in cd command
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
// Executes built in ln
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
// Executes built in rm
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
