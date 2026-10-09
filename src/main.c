#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;

    printf("Welcome to ShellForge!\n");
    printf("Type 'exit' to quit.\n");

    while (1)
    {
        printf("shellforge> ");
        fflush(stdout);

        length = getline(&line, &capacity, stdin);

        if (length == -1)
        {
            printf("\nExiting ShellForge...\n");
            break;
        }

        line[strcspn(line, "\n")] = '\0';

        if (strcmp(line, "exit") == 0)
        {
            printf("Exiting ShellForge...\n");
            break;
        }

        if (line[0] != '\0')
        {
            printf("You entered: %s\n", line);
        }
    }

    free(line);
    return 0;
}
