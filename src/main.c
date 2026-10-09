#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_TOKENS 64
#define DELIMITERS " \t\r\n\a"

// Tokenize input string into an argument vector
char **tokenize(char *line) {
    char **tokens = malloc(MAX_TOKENS * sizeof(char *));
    if (!tokens) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    int position = 0;
    char *token = strtok(line, DELIMITERS);

    while (token != NULL) {
        tokens[position++] = token;
        if (position >= MAX_TOKENS - 1) {
            break;
        }
        token = strtok(NULL, DELIMITERS);
    }
    tokens[position] = NULL;
    return tokens;
}

// Built-in cd implementation
int shellforge_cd(char **args) {
    if (args[1] == NULL) {
        char *home = getenv("HOME");
        if (home == NULL) {
            fprintf(stderr, "shellforge: cd: HOME not set\n");
            return 1;
        }
        if (chdir(home) != 0) {
            perror("shellforge: cd");
        }
    } else {
        if (chdir(args[1]) != 0) {
            perror("shellforge: cd");
        }
    }
    return 1;
}

// Handles input (<) and output (>) redirection before command execution
int execute_external(char **args) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    } else if (pid == 0) {
        // Child process: check for redirection tokens
        for (int i = 0; args[i] != NULL; i++) {
            // Output redirection: command > output.txt
            if (strcmp(args[i], ">") == 0) {
                if (args[i + 1] == NULL) {
                    fprintf(stderr, "shellforge: syntax error near unexpected token 'newline'\n");
                    exit(EXIT_FAILURE);
                }
                int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    perror("shellforge: open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
                args[i] = NULL; // Truncate args so execvp doesn't see '>'
                break;
            }
            // Input redirection: command < input.txt
            else if (strcmp(args[i], "<") == 0) {
                if (args[i + 1] == NULL) {
                    fprintf(stderr, "shellforge: syntax error near unexpected token 'newline'\n");
                    exit(EXIT_FAILURE);
                }
                int fd = open(args[i + 1], O_RDONLY);
                if (fd < 0) {
                    perror("shellforge: open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
                args[i] = NULL; // Truncate args so execvp doesn't see '<'
                break;
            }
        }

        if (execvp(args[0], args) == -1) {
            perror("shellforge");
        }
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
    return 1;
}

int execute_command(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        return shellforge_cd(args);
    }
    return execute_external(args);
}

int main(void) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read_bytes;

    while (1) {
        printf("shellforge> ");
        fflush(stdout);

        read_bytes = getline(&line, &len, stdin);

        if (read_bytes == -1) {
            printf("\n");
            break;
        }

        char **args = tokenize(line);

        if (args[0] == NULL) {
            free(args);
            continue;
        }

        if (strcmp(args[0], "exit") == 0) {
            free(args);
            break;
        }

        execute_command(args);

        free(args);
    }

    free(line);
    return 0;
}
