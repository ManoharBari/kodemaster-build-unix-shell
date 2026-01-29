#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024
#define MAX_ARGS 64
#define DELIMITERS " \t\r\n"

char **parse_line(char *line)
{
    static char *args[MAX_ARGS];
    int i = 0;

    char *token = strtok(line, DELIMITERS);
    while (token != NULL && i < MAX_ARGS - 1)
    {
        args[i++] = token;
        token = strtok(NULL, DELIMITERS);
    }
    args[i] = NULL;

    return args;
}

int execute_external(char **args)
{
    pid_t pid = fork();

    if (pid == 0)
    {
        // Child process
        if (execvp(args[0], args) == -1)
        {
            fprintf(stderr, "%s: command not found\n", args[0]);
            exit(127);
        }
    }
    else if (pid > 0)
    {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
    else
    {
        // Fork failed
        perror("fork");
    }

    return 1; // Continue shell loop
}

int handle_command(char **args)
{
    // Empty command
    if (args[0] == NULL)
    {
        return 1; // Continue loop
    }

    if (strcmp(args[0], "exit") == 0)
    {
        int exit_code = 0;

        // If exit code provided, use it
        if (args[1] != NULL)
        {
            exit_code = atoi(args[1]);
        }

        // Exit the shell process with the specified code
        exit(exit_code);
    }

    return execute_external(args);
}

int main(void)
{
    char input[BUFFER_SIZE];
    char **args;
    int status = 1;

    // REPL: Read-Eval-Print Loop
    while (status)
    {
        printf("$ ");
        fflush(stdout);

        // Read input
        if (fgets(input, BUFFER_SIZE, stdin) == NULL)
        {
            // EOF (Ctrl+D)
            printf("\n");
            break;
        }

        // Remove newline
        input[strcspn(input, "\n")] = '\0';

        // Skip empty lines
        if (strlen(input) == 0)
        {
            continue;
        }

        // Parse and execute
        args = parse_line(input);
        status = handle_command(args);
    }

    return 0;
}