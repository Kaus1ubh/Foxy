#include "foxy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int builtin_dispatch(char **tokens)
{
    if (!tokens || !tokens[0]) return 0;

    char *cmd = tokens[0];

    if (strcmp(cmd, "exit") == 0)
    {
        exit(0);
        return 1;
    }
    else if (strcmp(cmd, "cd") == 0)
    {
        if (tokens[1])
        {
            if (chdir(tokens[1]) != 0)
            {
                perror("foxy: cd");
            }
        }
        else
        {
            // Optional: cd to HOME if no args, but keeping it simple for now as per plan
            fprintf(stderr, "foxy: cd: missing argument\n");
        }
        return 1;
    }
    else if (strcmp(cmd, "help") == 0)
    {
        printf("Foxy Shell - Builtins:\n");
        printf("  cd <path>   Change directory\n");
        printf("  exit        Exit shell\n");
        printf("  help        Show this help\n");
        return 1;
    }

    return 0;
}
