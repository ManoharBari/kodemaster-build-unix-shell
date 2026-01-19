#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

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