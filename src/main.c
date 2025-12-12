#include "foxy.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#define MAX_LINE 1024

static char prompt_fmt[MAX_LINE] = "$CWD> ";

void set_prompt_format(const char *fmt)
{
    if (!fmt) return;
    strncpy(prompt_fmt, fmt, MAX_LINE - 1);
    prompt_fmt[MAX_LINE - 1] = '\0';
}

void print_prompt()
{
    char cwd_buf[PATH_MAX];
    char *cwd = getcwd(cwd_buf, sizeof(cwd_buf));
    if (!cwd) cwd = "unknown";

    const char *p = prompt_fmt;
    while (*p)
    {
        if (*p == '$' && strncmp(p, "$CWD", 4) == 0)
        {
            printf("%s", cwd);
            p += 4;
        }
        else if (*p == '\\' && *(p+1) == 'n')
        {
            printf("\n");
            p += 2;
        }
        else
        {
            putchar(*p);
            ++p;
        }
    }
    fflush(stdout);
}

void handle_sigint(int sig)
{
    (void)sig; // unused
    printf("\n");
    print_prompt(); 
}

#include "interaction.h"

// MAX_HISTORY code removed
// add_to_history removed
// read_line_with_history removed

void process_line(char *line)
{
    // Remove trailing newline
    line[strcspn(line, "\n")] = 0;

    // Empty line check
    if (line[0] == '\0') return;

    // Tokenize
    token_list_t tokens = {0};
    lex_err_t lex_err;
    if (tokenize_line(line, &tokens, &lex_err) != 0)
    {
        fprintf(stderr, "foxy: lex error %d\n", lex_err);
        return;
    }

    if (tokens.count == 0 || tokens.items[0] == NULL)
    {
        free_token_list(&tokens);
        return;
    }

    // Parse and Execute
    node_t *ast = parse_tokens(&tokens);
    if (ast)
    {
        exec_node(ast);
        free_ast(ast);
    }

    free_token_list(&tokens);
}

void run_rc_file()
{
    FILE *fp = fopen(".foxyrc", "r");
    if (fp)
    {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), fp))
        {
            process_line(line);
        }
        fclose(fp);
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


    puts("Foxy [Version 0.0.1]\n");

    // Run RC file
    run_rc_file();

    // 1b. Job Init
    job_init();

    // 1c. History Load
    history_init();
    history_load();

    while (1)
    {
        // 1c. Job Check
        job_check_status();

        // 2. Prompt
        print_prompt();

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
        }
        
        process_line(line_buf);
    }

    return 0;
}
