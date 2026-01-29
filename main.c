#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024
#define MAX_ARGS 64
#define DELIMITERS " \t\r\n"

char **parse_line(char *line) {
    static char *args[MAX_ARGS];
    int i = 0;
    
    char *token = strtok(line, DELIMITERS);
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, DELIMITERS);
    }
    args[i] = NULL;
    
    return args;
}

int execute_external(char **args) {
    pid_t pid = fork();
    
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "%s: command not found\n", args[0]);
            exit(127);
        }
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork");
    }
    
    return 1;
}

int handle_builtin(char **args) {
    // exit [code]
    if (strcmp(args[0], "exit") == 0) {
        int exit_code = 0;
        if (args[1] != NULL) {
            exit_code = atoi(args[1]);
        }
        exit(exit_code);
    }
    
    // Not a builtin
    return -1;
}

int execute(char **args) {
    if (args[0] == NULL) {
        return 1;
    }
    
    // Try builtins first
    int result = handle_builtin(args);
    if (result != -1) {
        return result;
    }
    
    // Execute as external command
    return execute_external(args);
}

int main(int argc, char *argv[]) {
    char input[BUFFER_SIZE];
    char **args;
    int status = 1;
    
    while (status) {
        printf("$ ");
        fflush(stdout);
        
        if (fgets(input, BUFFER_SIZE, stdin) == NULL) {
            printf("\n");
            break;
        }
        
        input[strcspn(input, "\n")] = '\0';
        
        if (strlen(input) == 0) {
            continue;
        }
        
        args = parse_line(input);
        status = execute(args);
    }
    
    return 0;
}
