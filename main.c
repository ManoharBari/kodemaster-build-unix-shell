#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

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

int execute(char **args) {
    if (args[0] == NULL) {
        return 1;  // Empty command
    }
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "%s: command not found\n", args[0]);
            exit(127);
        }
    } else if (pid < 0) {
        // Fork error
        perror("fork");
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
    
    return 1;
}

int main(int argc, char *argv[]) {
    char input[BUFFER_SIZE];
    
    while (1) {
        // Print prompt
        printf("$ ");
        fflush(stdout);
        
        // Read input
        if (fgets(input, BUFFER_SIZE, stdin) == NULL) {
            // EOF received (Ctrl+D)
            printf("\n");
            break;
        }
        
        // Remove trailing newline
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }
        
        // Skip empty lines
        if (strlen(input) == 0) {
            continue;
        }
        
        // For now, just echo the input
        printf("%s\n", input);
    }
    
    return 0;
}