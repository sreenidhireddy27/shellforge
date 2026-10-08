#include <stdio.h>

int main(void)
{
    printf("ShellForge started!\n");

    while (1)
    {
        char command[100];

        printf("shellforge> ");

        if (fgets(command, sizeof(command), stdin) == NULL)
        {
            break;
        }

        printf("You entered: %s", command);
    }

    return 0;
}
