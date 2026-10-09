#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKENS 64
#define DELIMITERS " \t\r\n\a"

// Tokenizes an input string into an array of arguments
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

int main(void) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read_bytes;

    while (1) {
        printf("shellforge> ");
        fflush(stdout);

        read_bytes = getline(&line, &len, stdin);

        // Handle Ctrl+D (EOF)
        if (read_bytes == -1) {
            printf("\n");
            break;
        }

        // Parse the input into tokens
        char **args = tokenize(line);

        // Empty line entered
        if (args[0] == NULL) {
            free(args);
            continue;
        }

        // Check for exit built-in
        if (strcmp(args[0], "exit") == 0) {
            free(args);
            break;
        }

        // Print parsed tokens to verify tokenization
        printf("Tokens parsed:\n");
        for (int i = 0; args[i] != NULL; i++) {
            printf("  arg[%d]: %s\n", i, args[i]);
        }

        free(args);
    }

    free(line);
    return 0;
}
