#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_TOKENS 64
#define DELIMITERS " \t\r\n\a"

// Global tracking for currently active child process
static pid_t foreground_pid = -1;

// Signal handler for SIGINT (Ctrl+C)
void handle_sigint(int sig) {
    (void)sig;
    if (foreground_pid > 0) {
        // Forward SIGINT to active child process
        kill(foreground_pid, SIGINT);
    } else {
        // No child running; write newline and redraw prompt safely
        write(STDOUT_FILENO, "\nshellforge> ", 13);
    }
}

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

// Apply I/O redirection (< and >) on an argument array
void handle_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
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
            args[i] = NULL;
            break;
        } else if (strcmp(args[i], "<") == 0) {
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
            args[i] = NULL;
            break;
        }
    }
}

// Execute piped commands: args1 | args2
int execute_pipeline(char **args1, char **args2) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return 1;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        // Child resets SIGINT to default
        signal(SIGINT, SIG_DFL);
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        handle_redirection(args1);
        if (execvp(args1[0], args1) == -1) {
            perror("shellforge");
        }
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        // Child resets SIGINT to default
        signal(SIGINT, SIG_DFL);
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        handle_redirection(args2);
        if (execvp(args2[0], args2) == -1) {
            perror("shellforge");
        }
        exit(EXIT_FAILURE);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 1;
}

// Execute external binary with foreground pid tracking
int execute_external(char **args) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    } else if (pid == 0) {
        // Child restores default SIGINT action so it can be terminated by Ctrl+C
        signal(SIGINT, SIG_DFL);
        handle_redirection(args);
        if (execvp(args[0], args) == -1) {
            perror("shellforge");
        }
        exit(EXIT_FAILURE);
    } else {
        foreground_pid = pid;
        int status;
        waitpid(pid, &status, 0);
        foreground_pid = -1;
    }
    return 1;
}

// Command dispatcher
int execute_command(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        return shellforge_cd(args);
    }

    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            args[i] = NULL;
            char **args2 = &args[i + 1];
            return execute_pipeline(args, args2);
        }
    }

    return execute_external(args);
}

int main(void) {
    // Configure sigaction for SIGINT
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

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
