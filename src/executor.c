#include "shellforge.h"

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
