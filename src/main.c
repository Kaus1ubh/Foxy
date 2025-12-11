#include "foxy.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#define MAX_LINE 1024

void handle_sigint(int sig)
{
    (void)sig; // unused
    printf("\n");
    // Reprint prompt?
    // For simple signal handling, just catching it prevents exit.
    // Ideally we would want to clear the current line or something, 
    // but standard behavior is just newline and maybe prompt.
    // Since we are in fgets, it might return NULL or be interrupted.
}

int main(void)
{
    // 1. Signal Handling
    // Ignore SIGINT (Ctrl+C) so the shell remains active
    if (signal(SIGINT, handle_sigint) == SIG_ERR)
    {
        perror("foxy: signal");
        exit(1);
    }

    char line_buf[MAX_LINE];
    char cwd_buf[PATH_MAX];

    puts("Foxy [Version 0.0.1]");

    while (1)
    {
        // 2. Prompt
        if (getcwd(cwd_buf, sizeof(cwd_buf)) != NULL)
        {
            printf("%s> ", cwd_buf);
        }
        else
        {
            printf("foxy> ");
        }
        fflush(stdout);

        // 3. Read Line
        if (fgets(line_buf, sizeof(line_buf), stdin) == NULL)
        {
            // EOF (Ctrl+D) or error
            if (feof(stdin))
            {
                printf("\n");
                break;
            }
            else
            {
                perror("foxy: read error");
                continue;
            }
        }

        // Remove trailing newline
        line_buf[strcspn(line_buf, "\n")] = 0;

        // Empty line check
        if (line_buf[0] == '\0') continue;

        // 4. Tokenize
        token_list_t tokens = {0};
        lex_err_t lex_err;
        if (tokenize_line(line_buf, &tokens, &lex_err) != 0)
        {
            fprintf(stderr, "foxy: lex error %d\n", lex_err);
            continue;
        }

        if (tokens.count == 0 || tokens.items[0] == NULL)
        {
            free_token_list(&tokens);
            continue;
        }

        // 5. Buildins / Exec
        if (!builtin_dispatch(tokens.items))
        {
            // Placeholder for external execution
            printf("[Exec] %s\n", tokens.items[0]);
        }

        free_token_list(&tokens);
    }

    return 0;
}
