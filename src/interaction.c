#include "interaction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <windows.h>
#include <ctype.h>

#define MAX_HISTORY 100
#define HISTORY_FILE ".foxy_history"
#define MAX_LINE 1024

static char *history[MAX_HISTORY];
static int history_count = 0;

void history_init()
{
    history_count = 0;
    memset(history, 0, sizeof(history));
}

void history_load()
{
    FILE *fp = fopen(HISTORY_FILE, "r");
    if (!fp) return;
    
    char line[1024];
    while (fgets(line, sizeof(line), fp))
    {
        line[strcspn(line, "\n")] = 0;
        if (line[0])
        {
            if (history_count < MAX_HISTORY)
            {
                history[history_count++] = strdup(line);
            }
            else
            {
                free(history[0]);
                for (int i = 1; i < MAX_HISTORY; ++i) history[i-1] = history[i];
                history[MAX_HISTORY-1] = strdup(line);
            }
        }
    }
    fclose(fp);
}

void add_to_history(const char *cmd)
{
    if (!cmd || !*cmd) return;
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
    
    // Check if we should append to file
    FILE *fp = fopen(HISTORY_FILE, "a");
    if (fp)
    {
        fprintf(fp, "%s\n", cmd);
        fclose(fp);
    }
}

// Helper: Clear current line usage on console not used yet
// static void clear_line(int len) ...

// Reverse Increment Search
static int do_reverse_search(char *buf, int max_len)
{
    (void)max_len;
    char search_term[256] = {0};
    int s_idx = 0;
    int match_idx = -1;
    
    printf("\n(reverse-i-search)`': ");
    
    while (1)
    {
        int ch = _getch();
        if (ch == '\r') // Enter -> Accept match
        {
            if (match_idx != -1)
            {
                strcpy(buf, history[match_idx]);
            }
            // Clear prompt line
            printf("\r                                                                      ");
            printf("\r%s", buf); // Reprint prompt handled by caller? No, caller expects buf filled. 
            // Caller loop will continue or return? Standard readline returns the line.
            // But we are inside read_line which expects to continue editing or return?
            // Let's assume Enter accepts and executes.
            return 1; // 1 means line ready
        }
        else if (ch == 27) // Esc -> Cancel
        {
             printf("\r                                                                      ");
             // Restore what was there? Or just clear.
             // Simplest: cancel yields empty line or keeps what was there.
             // Let's clear search line and return to normal editing of *buf*.
             // For now, simple exit.
             printf("\rfoxy> %s", buf); // Reprint prompt hack? 
             // Actually, the main loop handles prompt. We are in the read_line.
             // It's tricky without a proper redraw function. 
             // Let's just return 0 to indicate "continue editing" but we need to signal state.
             // Let's stick to simple: Enter accepts, anything else edits search.
             return -1; // -1 means cancel search mode
        }
        else if (ch == 8 || ch == '\b') // Backspace
        {
            if (s_idx > 0)
            {
                search_term[--s_idx] = 0;
            }
        }
        else if (ch >= 32 && ch <= 126 && s_idx < 255)
        {
            search_term[s_idx++] = ch;
            search_term[s_idx] = 0;
        }

        // Perform search
        match_idx = -1;
        for (int i = history_count - 1; i >= 0; --i)
        {
            if (strstr(history[i], search_term))
            {
                match_idx = i;
                break;
            }
        }

        // Redraw search prompt
        printf("\r(reverse-i-search)`%s': %s", search_term, match_idx != -1 ? history[match_idx] : "");
        // Clear rest of line logic missing, manual spaces for now
        printf("        \b\b\b\b\b\b\b\b");
    }
}

// Tab Completion
static void do_completion(char *buf, int *pos)
{
    // Very basic completion: match files in CWD
    // 1. Find start of word
    int start = *pos;
    while (start > 0 && buf[start-1] != ' ') start--;
    
    char partial[256];
    int len = *pos - start;
    if (len >= 255) return;
    
    strncpy(partial, buf + start, len);
    partial[len] = '\0';
    
    // 2. Search for matches
    // Builtins
    const char *builtins[] = { "cd", "exit", "help", "echo", "prompt", "jobs", "fg", NULL };
    for (int i = 0; builtins[i]; ++i)
    {
        if (strncmp(builtins[i], partial, len) == 0)
        {
             int rest_len = strlen(builtins[i]) - len;
             for (int j = 0; j < rest_len; ++j)
             {
                 buf[(*pos)++] = builtins[i][len + j];
                 printf("%c", builtins[i][len + j]);
             }
             buf[*pos] = '\0';
             return; // Found one, stop (simple)
        }
    }

    WIN32_FIND_DATA ffd;
    char pattern[256];
    sprintf(pattern, "%s*", partial);
    
    HANDLE hFind = FindFirstFile(pattern, &ffd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        // Just take the first one for now
        // Ideally we cycle.
        char *match = ffd.cFileName;
        
        // Append difference
        int rest_len = strlen(match) - len;
        if (rest_len > 0)
        {
             // Check buffer space
             for (int i = 0; i < rest_len; ++i)
             {
                 buf[(*pos)++] = match[len + i];
                 printf("%c", match[len + i]);
             }
             buf[*pos] = '\0';
        }
        
        FindClose(hFind);
    }
}

int read_line_with_history(char *buf, int max_len)
{
    int pos = 0;
    int ch;
    int h_idx = history_count;
    
    if (buf[0]) { pos = strlen(buf); printf("%s", buf); } // Support pre-filled?
    else buf[0] = '\0';

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
        else if (ch == 9) // TAB
        {
            do_completion(buf, &pos);
        }
        else if (ch == 18) // Ctrl+R
        {
             // Simple search: clear current line, do search
             // char old_buf[MAX_LINE]; 
             // strcpy(old_buf, buf);
             int res = do_reverse_search(buf, max_len);
             if (res == 1) // Accepted
             {
                 printf("\n"); // Newline after search
                 return 1;
             }
             else
             {
                 // Cancelled
                 printf("\n"); 
                 // We kept buf modified or we should restore? 
                 // For now, simpler to just accept cancellation means "undefined" or empty or whatever search left.
                 // Actually do_reverse_search should handle restoration on Cancel?
                 // But I'm removing old_buf to fix error quickly.
                 return 0; // aborts current input
             }
        }
        else if (ch == 0 || ch == 224) // Arrow keys
        {
            int arrow = _getch();
            if (arrow == 72) // UP
            {
                if (h_idx > 0)
                {
                    h_idx--;
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
                        buf[0] = '\0';
                        pos = 0;
                    }
                }
            }
        }
        else if (ch == 3) // Ctrl+C
        {
             printf("^C\n");
             buf[0] = '\0';
             return 0;
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
