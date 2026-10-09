#include "shellforge.h"

pid_t foreground_pid = -1;

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

        expand_variables(args);
        execute_command(args);

        free(args);
    }

    free_history();
    free(line);
    return 0;
}
