#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_TOKENS 64
#define DELIMITERS " \t\r\n\a"
#define MAX_HISTORY 100

static pid_t foreground_pid = -1;

// History state
static char *history_entries[MAX_HISTORY];
static int history_count = 0;

void add_history_entry(const char *cmd) {
    if (!cmd || strlen(cmd) == 0) return;
    if (history_count < MAX_HISTORY) {
        history_entries[history_count++] = strdup(cmd);
    } else {
        free(history_entries[0]);
        for (int i = 1; i < MAX_HISTORY; i++) {
            history_entries[i - 1] = history_entries[i];
        }
        history_entries[MAX_HISTORY - 1] = strdup(cmd);
    }
}

int shellforge_history(void) {
    for (int i = 0; i < history_count; i++) {
        printf("%4d  %s\n", i + 1, history_entries[i]);
    }
    return 1;
}

void free_history(void) {
    for (int i = 0; i < history_count; i++) {
        free(history_entries[i]);
    }
}

void handle_sigint(int sig) {
    (void)sig;
    if (foreground_pid > 0) {
        kill(foreground_pid, SIGINT);
    } else {
        write(STDOUT_FILENO, "\nshellforge> ", 13);
    }
}

void handle_sigchld(int sig) {
    (void)sig;
    int saved_errno = errno;
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    }
    errno = saved_errno;
}

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

void expand_variables(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (args[i][0] == '$' && args[i][1] != '\0') {
            char *var_name = &args[i][1];
            char *val = getenv(var_name);
            if (val != NULL) {
                args[i] = val;
            } else {
                args[i] = "";
            }
        }
    }
}

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

int execute_pipeline(char **args1, char **args2) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return 1;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
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

int execute_external(char **args, int in_background) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    } else if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        handle_redirection(args);
        if (execvp(args[0], args) == -1) {
            perror("shellforge");
        }
        exit(EXIT_FAILURE);
    } else {
        if (in_background) {
            printf("[Process running in background with PID %d]\n", pid);
        } else {
            foreground_pid = pid;
            int status;
            waitpid(pid, &status, 0);
            foreground_pid = -1;
        }
    }
    return 1;
}

int execute_command(char **args) {
    expand_variables(args);

    if (strcmp(args[0], "cd") == 0) {
        return shellforge_cd(args);
    }

    if (strcmp(args[0], "history") == 0) {
        return shellforge_history();
    }

    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            args[i] = NULL;
            char **args2 = &args[i + 1];
            return execute_pipeline(args, args2);
        }
    }

    int in_background = 0;
    int last_idx = 0;
    while (args[last_idx] != NULL) {
        last_idx++;
    }
    if (last_idx > 0 && strcmp(args[last_idx - 1], "&") == 0) {
        in_background = 1;
        args[last_idx - 1] = NULL;
    }

    return execute_external(args, in_background);
}

int main(void) {
    struct sigaction sa_int;
    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);

    struct sigaction sa_chld;
    memset(&sa_chld, 0, sizeof(sa_chld));
    sa_chld.sa_handler = handle_sigchld;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, NULL);

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

        // Strip newline for history record
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) > 0) {
            add_history_entry(line);
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

    free_history();
    free(line);
    return 0;
}
