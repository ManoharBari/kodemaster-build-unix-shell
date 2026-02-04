#define _POSIX_C_SOURCE 200809L
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
        if (execvp(args[0], args) == -1)
        {
            fprintf(stderr, "%s: command not found\n", args[0]);
            exit(127);
        }
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
    }
    else
    {
        perror("fork");
    }

    return 1;
}

int handle_builtin(char **args)
{
    // exit [code]
    if (strcmp(args[0], "exit") == 0)
    {
        int exit_code = 0;
        if (args[1] != NULL)
        {
            exit_code = atoi(args[1]);
        }
        exit(exit_code);
    }

    // echo [args...]
    if (strcmp(args[0], "echo") == 0)
    {
        // Print all arguments after "echo", separated by spaces
        for (int i = 1; args[i] != NULL; i++)
        {
            if (i > 1)
            {
                printf(" "); // Space between arguments
            }
            printf("%s", args[i]);
        }
        printf("\n"); // Newline at the end
        return 1;     // Continue shell loop
    }

    // pwd
    if (strcmp(args[0], "pwd") == 0)
    {
        char cwd[BUFFER_SIZE];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            printf("%s\n", cwd);
        }
        else
        {
            perror("pwd");
        }
        return 1;
    }

    // cd [directory]
    if (strcmp(args[0], "cd") == 0)
    {
        char *path;

        // No argument: go to HOME
        if (args[1] == NULL)
        {
            path = getenv("HOME");
            if (path == NULL)
            {
                fprintf(stderr, "cd: HOME not set\n");
                return 1;
            }
        }
        // Handle ~ (home directory)
        else if (strcmp(args[1], "~") == 0)
        {
            path = getenv("HOME");
            if (path == NULL)
            {
                fprintf(stderr, "cd: HOME not set\n");
                return 1;
            }
        }
        else
        {
            path = args[1];
        }

        // Change directory
        if (chdir(path) != 0)
        {
            perror("cd");
        }

        return 1;
    }

    // type <command>
    if (strcmp(args[0], "type") == 0)
    {
        if (args[1] == NULL)
        {
            fprintf(stderr, "type: missing argument\n");
            return 1;
        }

        // Check if it's a builtin
        if (strcmp(args[1], "exit") == 0 ||
            strcmp(args[1], "echo") == 0 ||
            strcmp(args[1], "pwd") == 0 ||
            strcmp(args[1], "cd") == 0 ||
            strcmp(args[1], "type") == 0)
        {
            printf("%s is a shell builtin\n", args[1]);
            return 1;
        }

        // Check if it's in PATH
        char *path = getenv("PATH");
        if (path == NULL)
        {
            printf("%s: not found\n", args[1]);
            return 1;
        }

        char *path_copy = strdup(path);
        if (path_copy == NULL)
        {
            perror("strdup");
            return 1;
        }

        char *dir = strtok(path_copy, ":");
        int found = 0;

        while (dir != NULL)
        {
            char full_path[BUFFER_SIZE];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir, args[1]);

            if (access(full_path, X_OK) == 0)
            {
                printf("%s is %s\n", args[1], full_path);
                found = 1;
                break;
            }
            dir = strtok(NULL, ":");
        }

        if (!found)
        {
            printf("%s: not found\n", args[1]);
        }

        free(path_copy);
        return 1;
    }

    // Not a builtin
    return -1;
}

int execute(char **args)
{
    if (args[0] == NULL)
    {
        return 1;
    }

    // Try builtins first
    int result = handle_builtin(args);
    if (result != -1)
    {
        return result;
    }

    // Execute as external command
    return execute_external(args);
}

int main(void)
{
    char input[BUFFER_SIZE];
    char **args;
    int status = 1;
    int interactive = isatty(STDIN_FILENO);

    while (status)
    {
        if (interactive)
        {
            printf("$ ");
            fflush(stdout);
        }

        if (fgets(input, BUFFER_SIZE, stdin) == NULL)
        {
            if (interactive)
            {
                printf("\n");
            }
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0)
        {
            continue;
        }

        args = parse_line(input);
        status = execute(args);
    }

    return 0;
}
