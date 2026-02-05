#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
#define MAX_ARGS 64
#define MAX_COMMANDS 64
#define HISTORY_SIZE 1000
#define DELIMITERS " \t\r\n"
#define SHELL_VERSION "1.0"

#define OP_NONE 0
#define OP_AND 1 // &&
#define OP_OR 2  // ||

// ANSI Color codes
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[1;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN "\033[1;36m"
#define COLOR_WHITE "\033[1;37m"

typedef struct
{
    char **args;
    char *stdin_file;
    char *stdout_file;
    char *stderr_file;
    int stdout_append;
    int stderr_append;
} Command;

typedef struct
{
    Command *commands;
    int num_commands;
    int operator;
} CommandGroup;

// History storage
char *history[HISTORY_SIZE];
int history_count = 0;

void print_banner(void)
{
    printf("\n");
    printf(COLOR_CYAN "╔════════════════════════════════════════════╗\n");
    printf("║                                            ║\n");
    printf("║        " COLOR_YELLOW "MyShell v%s" COLOR_CYAN "                      ║\n", SHELL_VERSION);
    printf("║                                            ║\n");
    printf("║  " COLOR_WHITE "A POSIX-compliant shell implementation" COLOR_CYAN "   ║\n");
    printf("║  " COLOR_GREEN "Type 'help' for available commands" COLOR_CYAN "      ║\n");
    printf("║                                            ║\n");
    printf("╚════════════════════════════════════════════╝\n" COLOR_RESET);
    printf("\n");
}

void print_prompt(void)
{
    char cwd[BUFFER_SIZE];
    char *user = getenv("USER");
    char *home = getenv("HOME");

    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        strcpy(cwd, "?");
    }

    // Replace home directory with ~
    if (home != NULL && strncmp(cwd, home, strlen(home)) == 0)
    {
        char temp[BUFFER_SIZE];
        snprintf(temp, sizeof(temp), "~%s", cwd + strlen(home));
        strcpy(cwd, temp);
    }

    printf(COLOR_GREEN "%s" COLOR_RESET ":" COLOR_BLUE "%s" COLOR_RESET "$ ",
           user ? user : "user", cwd);
    fflush(stdout);
}

void add_to_history(const char *cmd)
{
    if (cmd == NULL || strlen(cmd) == 0)
    {
        return;
    }

    if (history_count < HISTORY_SIZE)
    {
        history[history_count] = strdup(cmd);
        if (history[history_count] != NULL)
        {
            history_count++;
        }
    }
    else
    {
        free(history[0]);
        for (int i = 0; i < HISTORY_SIZE - 1; i++)
        {
            history[i] = history[i + 1];
        }
        history[HISTORY_SIZE - 1] = strdup(cmd);
    }
}

void load_history(void)
{
    char *home = getenv("HOME");
    if (home == NULL)
    {
        return;
    }

    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path), "%s/.myshell_history", home);

    FILE *f = fopen(path, "r");
    if (f == NULL)
    {
        return;
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), f) != NULL && history_count < HISTORY_SIZE)
    {
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) > 0)
        {
            history[history_count] = strdup(line);
            if (history[history_count] != NULL)
            {
                history_count++;
            }
        }
    }

    fclose(f);
}

void save_history(void)
{
    char *home = getenv("HOME");
    if (home == NULL)
    {
        return;
    }

    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path), "%s/.myshell_history", home);

    FILE *f = fopen(path, "w");
    if (f == NULL)
    {
        perror("save_history");
        return;
    }

    for (int i = 0; i < history_count; i++)
    {
        fprintf(f, "%s\n", history[i]);
    }

    fclose(f);
}

void free_history(void)
{
    for (int i = 0; i < history_count; i++)
    {
        free(history[i]);
        history[i] = NULL;
    }
    history_count = 0;
}

char **parse_line(char *line)
{
    static char *args[MAX_ARGS];
    static char tokens[MAX_ARGS][BUFFER_SIZE];
    int arg_idx = 0;
    int char_idx = 0;
    int in_single_quote = 0;
    int in_double_quote = 0;
    int i = 0;

    while (line[i] != '\0' && arg_idx < MAX_ARGS - 1)
    {
        char c = line[i];

        if (c == '\'' && !in_double_quote)
        {
            in_single_quote = !in_single_quote;
            i++;
            continue;
        }

        if (c == '"' && !in_single_quote)
        {
            in_double_quote = !in_double_quote;
            i++;
            continue;
        }

        if (c == '\\' && !in_single_quote)
        {
            char next = line[i + 1];

            if (in_double_quote)
            {
                if (next == '"' || next == '\\')
                {
                    if (char_idx < BUFFER_SIZE - 1)
                    {
                        tokens[arg_idx][char_idx++] = next;
                    }
                    i += 2;
                    continue;
                }
            }
            else
            {
                if (next != '\0')
                {
                    if (char_idx < BUFFER_SIZE - 1)
                    {
                        tokens[arg_idx][char_idx++] = next;
                    }
                    i += 2;
                    continue;
                }
            }
        }

        if (!in_single_quote && !in_double_quote && (c == ' ' || c == '\t'))
        {
            if (char_idx > 0)
            {
                tokens[arg_idx][char_idx] = '\0';
                args[arg_idx] = tokens[arg_idx];
                arg_idx++;
                char_idx = 0;
            }
            i++;
            continue;
        }

        if (char_idx < BUFFER_SIZE - 1)
        {
            tokens[arg_idx][char_idx++] = c;
        }
        i++;
    }

    if (char_idx > 0)
    {
        tokens[arg_idx][char_idx] = '\0';
        args[arg_idx] = tokens[arg_idx];
        arg_idx++;
    }

    args[arg_idx] = NULL;
    return args;
}

void parse_redirections(Command *cmd)
{
    cmd->stdin_file = NULL;
    cmd->stdout_file = NULL;
    cmd->stderr_file = NULL;
    cmd->stdout_append = 0;
    cmd->stderr_append = 0;

    for (int i = 0; cmd->args[i] != NULL; i++)
    {
        if (strcmp(cmd->args[i], "<") == 0)
        {
            if (cmd->args[i + 1] != NULL)
            {
                cmd->stdin_file = cmd->args[i + 1];
                cmd->args[i] = NULL;
            }
        }
        else if (strcmp(cmd->args[i], ">>") == 0 || strcmp(cmd->args[i], "1>>") == 0)
        {
            if (cmd->args[i + 1] != NULL)
            {
                cmd->stdout_file = cmd->args[i + 1];
                cmd->stdout_append = 1;
                cmd->args[i] = NULL;
            }
        }
        else if (strcmp(cmd->args[i], ">") == 0 || strcmp(cmd->args[i], "1>") == 0)
        {
            if (cmd->args[i + 1] != NULL)
            {
                cmd->stdout_file = cmd->args[i + 1];
                cmd->stdout_append = 0;
                cmd->args[i] = NULL;
            }
        }
        else if (strcmp(cmd->args[i], "2>>") == 0)
        {
            if (cmd->args[i + 1] != NULL)
            {
                cmd->stderr_file = cmd->args[i + 1];
                cmd->stderr_append = 1;
                cmd->args[i] = NULL;
            }
        }
        else if (strcmp(cmd->args[i], "2>") == 0)
        {
            if (cmd->args[i + 1] != NULL)
            {
                cmd->stderr_file = cmd->args[i + 1];
                cmd->stderr_append = 0;
                cmd->args[i] = NULL;
            }
        }
    }
}

int is_builtin(char *cmd)
{
    return (strcmp(cmd, "exit") == 0 ||
            strcmp(cmd, "echo") == 0 ||
            strcmp(cmd, "pwd") == 0 ||
            strcmp(cmd, "cd") == 0 ||
            strcmp(cmd, "type") == 0 ||
            strcmp(cmd, "history") == 0 ||
            strcmp(cmd, "help") == 0 ||
            strcmp(cmd, "clear") == 0);
}

int builtin_help(void)
{
    printf("\n" COLOR_CYAN "MyShell v%s - Built-in Commands\n" COLOR_RESET "\n", SHELL_VERSION);

    printf(COLOR_YELLOW "Navigation & Files:\n" COLOR_RESET);
    printf("  " COLOR_GREEN "cd [dir]" COLOR_RESET "      Change directory (no arg = HOME)\n");
    printf("  " COLOR_GREEN "pwd" COLOR_RESET "           Print working directory\n");
    printf("\n");

    printf(COLOR_YELLOW "Information:\n" COLOR_RESET);
    printf("  " COLOR_GREEN "type <cmd>" COLOR_RESET "   Show command type and location\n");
    printf("  " COLOR_GREEN "history" COLOR_RESET "      Show command history\n");
    printf("  " COLOR_GREEN "help" COLOR_RESET "         Show this help message\n");
    printf("\n");

    printf(COLOR_YELLOW "Output:\n" COLOR_RESET);
    printf("  " COLOR_GREEN "echo [text]" COLOR_RESET "  Print text to stdout\n");
    printf("  " COLOR_GREEN "clear" COLOR_RESET "        Clear the screen\n");
    printf("\n");

    printf(COLOR_YELLOW "Shell Control:\n" COLOR_RESET);
    printf("  " COLOR_GREEN "exit [code]" COLOR_RESET "  Exit shell (default code: 0)\n");
    printf("\n");

    printf(COLOR_CYAN "Features:\n" COLOR_RESET);
    printf("  • Pipes: " COLOR_GREEN "cmd1 | cmd2 | cmd3\n" COLOR_RESET);
    printf("  • Redirects: " COLOR_GREEN "> >> < 2> 2>>\n" COLOR_RESET);
    printf("  • Logical: " COLOR_GREEN "&& ||\n" COLOR_RESET);
    printf("  • Quotes: " COLOR_GREEN "'single' \"double\" \\\n" COLOR_RESET);
    printf("\n");

    printf(COLOR_YELLOW "Examples:\n" COLOR_RESET);
    printf("  " COLOR_GREEN "ls | grep txt > files.txt\n" COLOR_RESET);
    printf("  " COLOR_GREEN "cat file.txt 2> errors.log\n" COLOR_RESET);
    printf("  " COLOR_GREEN "mkdir test && cd test && pwd\n" COLOR_RESET);
    printf("  " COLOR_GREEN "echo 'Hello World'\n" COLOR_RESET);
    printf("\n");

    return 0;
}

int execute_builtin(Command *cmd)
{
    char **args = cmd->args;

    int saved_stdin = -1;
    int saved_stdout = -1;
    int saved_stderr = -1;

    if (cmd->stdin_file != NULL)
    {
        int fd = open(cmd->stdin_file, O_RDONLY);
        if (fd < 0)
        {
            perror(cmd->stdin_file);
            return 1;
        }

        saved_stdin = dup(STDIN_FILENO);
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (cmd->stdout_file != NULL)
    {
        int flags = O_WRONLY | O_CREAT;
        flags |= cmd->stdout_append ? O_APPEND : O_TRUNC;

        int fd = open(cmd->stdout_file, flags, 0644);
        if (fd < 0)
        {
            perror("open");
            if (saved_stdin >= 0)
            {
                dup2(saved_stdin, STDIN_FILENO);
                close(saved_stdin);
            }
            return 1;
        }

        saved_stdout = dup(STDOUT_FILENO);
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    if (cmd->stderr_file != NULL)
    {
        int flags = O_WRONLY | O_CREAT;
        flags |= cmd->stderr_append ? O_APPEND : O_TRUNC;

        int fd = open(cmd->stderr_file, flags, 0644);
        if (fd < 0)
        {
            perror("open");
            if (saved_stdin >= 0)
            {
                dup2(saved_stdin, STDIN_FILENO);
                close(saved_stdin);
            }
            if (saved_stdout >= 0)
            {
                dup2(saved_stdout, STDOUT_FILENO);
                close(saved_stdout);
            }
            return 1;
        }

        saved_stderr = dup(STDERR_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
    }

    int result = 0;

    if (strcmp(args[0], "exit") == 0)
    {
        save_history();
        free_history();

        printf(COLOR_CYAN "\nGoodbye! 👋\n" COLOR_RESET);

        int exit_code = 0;
        if (args[1] != NULL)
        {
            exit_code = atoi(args[1]);
        }
        exit(exit_code);
    }
    else if (strcmp(args[0], "help") == 0)
    {
        result = builtin_help();
    }
    else if (strcmp(args[0], "clear") == 0)
    {
        printf("\033[2J\033[H");
        fflush(stdout);
    }
    else if (strcmp(args[0], "history") == 0)
    {
        for (int i = 0; i < history_count; i++)
        {
            printf("%4d  %s\n", i + 1, history[i]);
        }
    }
    else if (strcmp(args[0], "echo") == 0)
    {
        for (int i = 1; args[i] != NULL; i++)
        {
            if (i > 1)
            {
                printf(" ");
            }
            printf("%s", args[i]);
        }
        printf("\n");
    }
    else if (strcmp(args[0], "pwd") == 0)
    {
        char cwd[BUFFER_SIZE];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            printf("%s\n", cwd);
        }
        else
        {
            perror("pwd");
            result = 1;
        }
    }
    else if (strcmp(args[0], "cd") == 0)
    {
        char *path;

        if (args[1] == NULL)
        {
            path = getenv("HOME");
            if (path == NULL)
            {
                fprintf(stderr, "cd: HOME not set\n");
                result = 1;
                goto cleanup;
            }
        }
        else if (strcmp(args[1], "~") == 0)
        {
            path = getenv("HOME");
            if (path == NULL)
            {
                fprintf(stderr, "cd: HOME not set\n");
                result = 1;
                goto cleanup;
            }
        }
        else
        {
            path = args[1];
        }

        if (chdir(path) != 0)
        {
            perror("cd");
            result = 1;
        }
    }
    else if (strcmp(args[0], "type") == 0)
    {
        if (args[1] == NULL)
        {
            fprintf(stderr, "type: missing argument\n");
            result = 1;
            goto cleanup;
        }

        if (is_builtin(args[1]))
        {
            printf("%s is a shell builtin\n", args[1]);
            goto cleanup;
        }

        char *path = getenv("PATH");
        if (path == NULL)
        {
            printf("%s: not found\n", args[1]);
            result = 1;
            goto cleanup;
        }

        char *path_copy = strdup(path);
        if (path_copy == NULL)
        {
            perror("strdup");
            result = 1;
            goto cleanup;
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
            result = 1;
        }

        free(path_copy);
    }

cleanup:
    if (saved_stdin >= 0)
    {
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
    }

    if (saved_stdout >= 0)
    {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }

    if (saved_stderr >= 0)
    {
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);
    }

    return result;
}

void execute_command(Command *cmd, int input_fd, int output_fd)
{
    if (input_fd != STDIN_FILENO)
    {
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }

    if (output_fd != STDOUT_FILENO)
    {
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }

    if (cmd->stdin_file != NULL)
    {
        int fd = open(cmd->stdin_file, O_RDONLY);
        if (fd < 0)
        {
            perror(cmd->stdin_file);
            exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (cmd->stdout_file != NULL)
    {
        int flags = O_WRONLY | O_CREAT;
        flags |= cmd->stdout_append ? O_APPEND : O_TRUNC;

        int fd = open(cmd->stdout_file, flags, 0644);
        if (fd < 0)
        {
            perror("open");
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    if (cmd->stderr_file != NULL)
    {
        int flags = O_WRONLY | O_CREAT;
        flags |= cmd->stderr_append ? O_APPEND : O_TRUNC;

        int fd = open(cmd->stderr_file, flags, 0644);
        if (fd < 0)
        {
            perror("open");
            exit(1);
        }
        dup2(fd, STDERR_FILENO);
        close(fd);
    }

    execvp(cmd->args[0], cmd->args);
    fprintf(stderr, "%s: command not found\n", cmd->args[0]);
    exit(127);
}

int execute_pipeline(Command *commands, int num_commands)
{
    if (num_commands == 1)
    {
        if (is_builtin(commands[0].args[0]))
        {
            return execute_builtin(&commands[0]);
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            execute_command(&commands[0], STDIN_FILENO, STDOUT_FILENO);
        }
        else if (pid > 0)
        {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status))
            {
                return WEXITSTATUS(status);
            }
            return 1;
        }
        else
        {
            perror("fork");
        }
        return 1;
    }

    int prev_pipe_read = STDIN_FILENO;
    pid_t pids[MAX_COMMANDS];

    for (int i = 0; i < num_commands; i++)
    {
        int pipe_fd[2];

        if (i < num_commands - 1)
        {
            if (pipe(pipe_fd) < 0)
            {
                perror("pipe");
                return 1;
            }
        }

        pids[i] = fork();

        if (pids[i] == 0)
        {
            int input_fd = prev_pipe_read;
            int output_fd = (i < num_commands - 1) ? pipe_fd[1] : STDOUT_FILENO;

            if (i < num_commands - 1)
            {
                close(pipe_fd[0]);
            }

            execute_command(&commands[i], input_fd, output_fd);
        }
        else if (pids[i] < 0)
        {
            perror("fork");
            return 1;
        }

        if (prev_pipe_read != STDIN_FILENO)
        {
            close(prev_pipe_read);
        }

        if (i < num_commands - 1)
        {
            close(pipe_fd[1]);
            prev_pipe_read = pipe_fd[0];
        }
    }

    int last_status = 0;
    for (int i = 0; i < num_commands; i++)
    {
        int status;
        waitpid(pids[i], &status, 0);
        if (i == num_commands - 1 && WIFEXITED(status))
        {
            last_status = WEXITSTATUS(status);
        }
    }

    return last_status;
}

int execute(char **args)
{
    if (args[0] == NULL)
    {
        return 0;
    }

    // Split by logical operators (&& and ||)
    static CommandGroup groups[MAX_COMMANDS];
    int num_groups = 0;
    int group_start = 0;

    groups[0].operator = OP_NONE;

    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], "&&") == 0)
        {
            args[i] = NULL;
            num_groups++;
            group_start = i + 1;
            if (num_groups < MAX_COMMANDS)
            {
                groups[num_groups].operator = OP_AND;
            }
        }
        else if (strcmp(args[i], "||") == 0)
        {
            args[i] = NULL;
            num_groups++;
            group_start = i + 1;
            if (num_groups < MAX_COMMANDS)
            {
                groups[num_groups].operator = OP_OR;
            }
        }
    }
    num_groups++;

    int last_exit_status = 0;
    group_start = 0;

    for (int g = 0; g < num_groups; g++)
    {
        if (groups[g].operator == OP_AND && last_exit_status != 0)
        {
            while (group_start < MAX_ARGS && args[group_start] != NULL)
            {
                group_start++;
            }
            group_start++;
            continue;
        }
        else if (groups[g].operator == OP_OR && last_exit_status == 0)
        {
            while (group_start < MAX_ARGS && args[group_start] != NULL)
            {
                group_start++;
            }
            group_start++;
            continue;
        }

        static Command commands[MAX_COMMANDS];
        int num_commands = 0;
        int arg_start = group_start;

        for (int i = group_start;; i++)
        {
            if (args[i] == NULL || strcmp(args[i], "|") == 0)
            {
                if (i > arg_start && num_commands < MAX_COMMANDS)
                {
                    commands[num_commands].args = &args[arg_start];
                    if (args[i] != NULL && strcmp(args[i], "|") == 0)
                    {
                        args[i] = NULL;
                    }
                    parse_redirections(&commands[num_commands]);
                    num_commands++;
                    arg_start = i + 1;
                }

                if (args[i] == NULL)
                {
                    group_start = i + 1;
                    break;
                }
            }
        }

        if (num_commands > 0)
        {
            last_exit_status = execute_pipeline(commands, num_commands);
        }
    }

    return last_exit_status;
}

int main(void)
{
    char input[BUFFER_SIZE];
    char **args;
    int interactive = isatty(STDIN_FILENO);

    load_history();

    if (interactive)
    {
        print_banner();
    }

    while (1)
    {
        if (interactive)
        {
            print_prompt();
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

        add_to_history(input);

        args = parse_line(input);
        execute(args);
    }

    save_history();
    free_history();

    return 0;
}