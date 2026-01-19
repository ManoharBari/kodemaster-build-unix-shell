#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define MAX_ARGS 64
#define DELIMITERS " \t\r\n"

/* Parse input line into arguments */
void parse_line(char *line, char **args) {
    int i = 0;
    char *token = strtok(line, DELIMITERS);

    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, DELIMITERS);
    }
    args[i] = NULL;
}

/* Execute external command */
int execute_external(char **args) {
    pid_t pid = fork();

    if (pid == 0) {
        execvp(args[0], args);

        /* execvp returns only on error */
        fprintf(stderr, "%s: command not found\n", args[0]);
        _exit(127);
    }
    else if (pid > 0) {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
        }

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
    }
    else {
        perror("fork");
    }

    return 1;
}

/* Handle built-in commands */
int handle_builtin(char **args) {
    if (strcmp(args[0], "exit") == 0) {
        int exit_code = 0;

        if (args[1]) {
            char *endptr;
            long val = strtol(args[1], &endptr, 10);

            if (*endptr != '\0') {
                fprintf(stderr, "exit: numeric argument required\n");
                return 1;
            }
            exit_code = (int)val;
        }

        exit(exit_code);
    }

    return -1; // Not a builtin
}

int execute(char **args) {
    if (args[0] == NULL)
        return 1;

    int builtin_status = handle_builtin(args);
    if (builtin_status != -1)
        return builtin_status;

    return execute_external(args);
}

int main(void) {
    char input[BUFFER_SIZE];
    char *args[MAX_ARGS];
    int status = 1;

    while (status) {
        printf("$ ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        if (input[0] == '\n')
            continue;

        parse_line(input, args);
        status = execute(args);
    }

    return 0;
}
