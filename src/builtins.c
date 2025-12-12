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
            fprintf(stderr, "foxy: cd: missing argument\n");
        }
        return 1;
    }
    else if (strcmp(cmd, "help") == 0)
    {
        printf("Foxy Shell - Version 0.0.1\n\n");
        printf("CD       Change the current directory.\n");
        printf("ECHO     Display messages.\n");
        printf("EXIT     Quits the Foxy shell.\n");
        printf("FG       Brings a background job to the foreground (fg %%id).\n");
        printf("HELP     Provides Help information for Foxy commands.\n");
        printf("JOBS     Lists active background jobs.\n");
        printf("PROMPT   Customize the shell prompt (e.g., prompt $CWD> ).\n");
        printf("\nExternal commands (ping, whoami, etc.) are executed from the system PATH.\n");
        return 1;
    }
    else if (strcmp(cmd, "echo") == 0)
    {
        for (int i = 1; tokens[i]; ++i)
        {
            printf("%s%s", tokens[i], tokens[i+1] ? " " : "");
        }
        printf("\n");
        fflush(stdout);
        return 1;
    }
    else if (strcmp(cmd, "prompt") == 0)
    {
        char buf[1024] = "";
        for (int i = 1; tokens[i]; ++i)
        {
            strcat(buf, tokens[i]);
            if (tokens[i+1]) strcat(buf, " ");
        }
        // If empty, reset to default? Or allow empty?
        // Let's assume user wants what they typed. If nothing, maybe just empty string.
        // But better to at least have a space if they just typed "prompt" with nothing?
        // Cmd behavior: "prompt" resets to default.
        if (!tokens[1]) 
        {
            set_prompt_format("$CWD> ");
        }
        else
        {
            set_prompt_format(buf);
        }
        return 1;
    }
    else if (strcmp(cmd, "jobs") == 0)
    {
        job_print_all();
        return 1;
    }
    else if (strcmp(cmd, "fg") == 0)
    {
        if (tokens[1])
        {
            int id = atoi(tokens[1]); 
            if (tokens[1][0] == '%') id = atoi(tokens[1]+1);
            if (job_to_foreground(id) != 0)
            {
                // Error printed by job_to_foreground
            }
        }
        else
        {
             fprintf(stderr, "foxy: fg: missing job id\n");
        }
        return 1;
    }

    return 0;
}
