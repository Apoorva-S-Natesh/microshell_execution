#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64

void execute_command(char **args, char *input_file, char *output_file, int append_output) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process

        // Handle input redirection
        if (input_file != NULL) {
            int fd = open(input_file, O_RDONLY);
            if (fd == -1) {
                perror("open input file");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDIN_FILENO) == -1) {
                perror("dup2 input");
                exit(EXIT_FAILURE);
            }
            close(fd);
        }

        // Handle output redirection
        if (output_file != NULL) {
            int flags = O_WRONLY | O_CREAT;
            if (append_output) {
                flags |= O_APPEND;
            } else {
                flags |= O_TRUNC;
            }
            int fd = open(output_file, flags, 0644);
            if (fd == -1) {
                perror("open output file");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("dup2 output");
                exit(EXIT_FAILURE);
            }
            close(fd);
        }

        // Execute the command
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
}

int main() {
    char input[MAX_COMMAND_LENGTH];
    char *args[MAX_ARGS];
    char *input_file = NULL;
    char *output_file = NULL;
    int append_output = 0;

    while (1) {
        printf("shell> ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        // Remove newline
        input[strcspn(input, "\n")] = 0;

        // Tokenize input
        int arg_count = 0;
        char *token = strtok(input, " ");
        while (token != NULL && arg_count < MAX_ARGS - 1) {
            if (strcmp(token, "<") == 0) {
                token = strtok(NULL, " ");
                if (token != NULL) {
                    input_file = token;
                }
            } else if (strcmp(token, ">") == 0) {
                token = strtok(NULL, " ");
                if (token != NULL) {
                    output_file = token;
                    append_output = 0;
                }
            } else if (strcmp(token, ">>") == 0) {
                token = strtok(NULL, " ");
                if (token != NULL) {
                    output_file = token;
                    append_output = 1;
                }
            } else {
                args[arg_count++] = token;
            }
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;

        if (arg_count > 0) {
            if (strcmp(args[0], "exit") == 0) {
                break;
            }
            execute_command(args, input_file, output_file, append_output);
        }

        // Reset redirection variables
        input_file = NULL;
        output_file = NULL;
        append_output = 0;
    }

    return 0;
}
