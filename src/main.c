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

#include <conio.h>

#define MAX_HISTORY 100
static char *history[MAX_HISTORY];
static int history_count = 0;
static int history_pos = 0;

void add_to_history(const char *cmd)
{
    if (!cmd || !*cmd) return;
    // Don't add duplicates of the very last command
    if (history_count > 0 && strcmp(history[history_count-1], cmd) == 0) return;

    if (history_count < MAX_HISTORY)
    {
        history[history_count++] = strdup(cmd);
    }
    else
    {
        free(history[0]);
        for (int i = 1; i < MAX_HISTORY; ++i) history[i-1] = history[i];
        history[MAX_HISTORY-1] = strdup(cmd);
    }
}

int read_line_with_history(char *buf, int max_len)
{
    int pos = 0;
    int ch;
    int h_idx = history_count;
    char temp_buf[MAX_LINE]; // Store current typing when moving up/down

    temp_buf[0] = '\0';
    buf[0] = '\0';

    while (1)
    {
        ch = _getch();

        if (ch == '\r') // Enter
        {
            printf("\n");
            buf[pos] = '\0';
            return 1;
        }
        else if (ch == '\b' || ch == 8) // Backspace
        {
            if (pos > 0)
            {
                pos--;
                printf("\b \b");
                buf[pos] = '\0';
            }
        }
        else if (ch == 0 || ch == 224) // Arrow keys
        {
            int arrow = _getch();
            if (arrow == 72) // UP
            {
                if (h_idx > 0)
                {
                    // If we were at the end (new line), save it? 
                    // Simplifying: just navigate history
                    h_idx--;
                    // Clear line
                    while (pos > 0) { printf("\b \b"); pos--; }
                    strcpy(buf, history[h_idx]);
                    pos = strlen(buf);
                    printf("%s", buf);
                }
            }
            else if (arrow == 80) // DOWN
            {
                if (h_idx < history_count)
                {
                    h_idx++;
                    while (pos > 0) { printf("\b \b"); pos--; }
                    if (h_idx < history_count)
                    {
                        strcpy(buf, history[h_idx]);
                        pos = strlen(buf);
                        printf("%s", buf);
                    }
                    else
                    {
                        // Back to empty/temp
                        buf[0] = '\0';
                        pos = 0;
                    }
                }
            }
        }
        else if (ch == 3) // Ctrl+C handled via signal usually, but _getch might capture it
        {
             printf("^C\n");
             buf[0] = '\0';
             return 0; // abort
        }
        else if (pos < max_len - 1 && ch >= 32 && ch <= 126)
        {
            buf[pos++] = ch;
            buf[pos] = '\0';
            printf("%c", ch);
        }
        else if (ch == EOF)
        {
            return 0;
        }
    }
}

int main(void)
{
    // 1. Signal Handling
    if (signal(SIGINT, handle_sigint) == SIG_ERR)
    {
        perror("foxy: signal");
        exit(1);
    }

    char line_buf[MAX_LINE];
    char cwd_buf[PATH_MAX];

    puts("Foxy [Version 0.0.1]\n");

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
        if (_isatty(_fileno(stdin)))
        {
             // Interactive mode
             if (!read_line_with_history(line_buf, MAX_LINE))
             {
                 break; 
             }
             add_to_history(line_buf); 
        }
        else
        {
             // Script/Piped mode
             if (fgets(line_buf, sizeof(line_buf), stdin) == NULL)
             {
                 break;
             }
             // Remove trailing newline
             line_buf[strcspn(line_buf, "\n")] = 0;
        }

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

        // 5. Parse and Execute
        node_t *ast = parse_tokens(&tokens);
        if (ast)
        {
            exec_node(ast);
            free_ast(ast);
        }

        free_token_list(&tokens);
    }

    return 0;
}
