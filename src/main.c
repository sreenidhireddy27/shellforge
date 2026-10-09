#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_TOKENS 64
#define DELIMITERS " \t\r\n\a"

// Tokenizes user input line into an argument array
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

// Spawns a child process to run external programs
int execute_command(char **args) {
    pid_t pid = fork();

    if (pid < 0) {
        // Fork error
        perror("fork failed");
        return 1;
    } else if (pid == 0) {
        // Child process: execute the binary
        if (execvp(args[0], args) == -1) {
            perror("shellforge");
        }
        exit(EXIT_FAILURE);
    } else {
        // Parent process: wait for child to finish execution
        int status;
        waitpid(pid, &status, 0);
    }
    return 1;
}

int main(void) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read_bytes;

    while (1) {
        printf("shellforge> ");
        fflush(stdout);

        read_bytes = getline(&line, &len, stdin);

        // Exit on Ctrl+D (EOF)
        if (read_bytes == -1) {
            printf("\n");
            break;
        }

        char **args = tokenize(line);

        // Ignore empty input
        if (args[0] == NULL) {
            free(args);
            continue;
        }

        // Built-in exit command
        if (strcmp(args[0], "exit") == 0) {
            free(args);
            break;
        }

        // Execute external command
        execute_command(args);

        free(args);
    }

    free(line);
    return 0;
}
