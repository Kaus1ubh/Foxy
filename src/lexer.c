#include "foxy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INITIAL_TOK_CAP 16
#define INITIAL_BUF_CAP 128

static void free_token_list_internal(token_list_t *tlist) 
{
    if (!tlist) return;
    for (size_t i = 0; i < tlist->count; ++i) free(tlist->items[i]);
    free(tlist->items);
    tlist->items = NULL;
    tlist->count = 0;
}

/* expose for header */
void free_token_list(token_list_t *t)
{
    free_token_list_internal(t);
}

/* to add token */
static int tlist_add(token_list_t *tlist, const char *s) 
{
    if (!tlist->items) 
    {
        tlist->items = malloc(sizeof(char*) * INITIAL_TOK_CAP);

        if (!tlist->items) return -1;
        for (int i=0;i<INITIAL_TOK_CAP;i++) tlist->items[i] = NULL;

    }

    /* find allocated slots by walking until NULL sentinel */
    size_t slots = 0;
    while (tlist->items[slots]) ++slots;
    if (tlist->count + 2 > slots)
    {

        size_t new_slots = (slots == 0) ? INITIAL_TOK_CAP : slots * 2;
        char **tmp = realloc(tlist->items, sizeof(char*) * new_slots);
        if (!tmp) return -1;
        for (size_t i = slots; i < new_slots; ++i) tmp[i] = NULL;
        tlist->items = tmp;
        
    }
    tlist->items[tlist->count] = strdup(s ? s : "");
    if (!tlist->items[tlist->count]) return -1;
    tlist->count++;
    tlist->items[tlist->count] = NULL;
    return 0;
}

static int buf_append(char **buf, size_t *len, size_t *cap, char c) 
{
    if (*len + 1 >= *cap) 
    {

        size_t newcap = (*cap == 0) ? INITIAL_BUF_CAP : (*cap * 2);
        char *tmp = realloc(*buf, newcap);
        if (!tmp) return -1;
        *buf = tmp;
        *cap = newcap;

    }

    (*buf)[(*len)++] = c;

    return 0;

}

static int buf_push_token(char **buf, size_t *len, size_t *cap, token_list_t *tlist) 
{
    if (*len == 0) return 0;
    if (buf_append(buf, len, cap, '\0') < 0) return -1;
    int r = tlist_add(tlist, *buf);
    free(*buf);
    *buf = NULL;
    *len = 0;
    *cap = 0;
    return r;
}

static int is_special_char(char c) 
{
    return (c == '|' || c == '<' || c == '>' || c == '&' || c == ';');
}

int tokenize_line(const char *line, token_list_t *out, lex_err_t *errcode) 
{
    *out = (token_list_t){ .items = NULL, .count = 0 };
    *errcode = LEX_OK;
    if (!line) return 0;

    const char *p = line;
    char *buf = NULL;
    size_t blen = 0, bcap = 0;

    enum { S_NORMAL, S_SQUOTE, S_DQUOTE, S_ESC } state = S_NORMAL;

    while (*p) {
        char c = *p;

        if (state == S_ESC) {
            if (buf_append(&buf, &blen, &bcap, c) < 0) { *errcode = LEX_ERR_OOM; free(buf); return -1; }
            state = S_NORMAL;
            ++p;
            continue;
        }

        if (state == S_SQUOTE) {
            if (c == '\'') { state = S_NORMAL; ++p; continue; }
            if (buf_append(&buf, &blen, &bcap, c) < 0) { *errcode = LEX_ERR_OOM; free(buf); return -1; }
            ++p;
            continue;
        }

        if (state == S_DQUOTE) {
            if (c == '"') { state = S_NORMAL; ++p; continue; }
            if (c == '\\') {
                ++p;
                if (!*p) { *errcode = LEX_ERR_UNCLOSED_QUOTE; free(buf); return -1; }
                char nc = *p;
                if (buf_append(&buf, &blen, &bcap, nc) < 0) { *errcode = LEX_ERR_OOM; free(buf); return -1; }
                ++p;
                continue;
            }
            if (buf_append(&buf, &blen, &bcap, c) < 0) { *errcode = LEX_ERR_OOM; free(buf); return -1; }
            ++p;
            continue;
        }

        /* S_NORMAL */
        if (isspace((unsigned char)c)) 
        {
            if (buf_push_token(&buf, &blen, &bcap, out) < 0)
            {
                *errcode = LEX_ERR_OOM; free(buf); return -1; 
            }
            ++p;
            continue;
        }

        if (c == '\\') { state = S_ESC; ++p; continue; }
        if (c == '\'') { state = S_SQUOTE; ++p; continue; }
        if (c == '"') { state = S_DQUOTE; ++p; continue; }

        if (is_special_char(c)) {
            if (buf_push_token(&buf, &blen, &bcap, out) < 0)
            {
                *errcode = LEX_ERR_OOM; free(buf); return -1; 
            }
            if (c == '>' && *(p+1) == '>') 
            {
                if (tlist_add(out, ">>") < 0) { *errcode = LEX_ERR_OOM; return -1; 
            }
                p += 2;
                continue;
            }
             else 
            {
                char tmp[2] = { c, '\0' };
                if (tlist_add(out, tmp) < 0)
                {
                     *errcode = LEX_ERR_OOM; return -1; 
                }
                ++p;
                continue;
            }
        }

        if (buf_append(&buf, &blen, &bcap, c) < 0) 
        {
            *errcode = LEX_ERR_OOM; free(buf); return -1; 
        }
        ++p;
    }

    if (state == S_SQUOTE || state == S_DQUOTE || state == S_ESC) {
        *errcode = LEX_ERR_UNCLOSED_QUOTE;
        free(buf);
        free_token_list(out);
        return -1;
    }

    if (buf_push_token(&buf, &blen, &bcap, out) < 0) 
    {
         *errcode = LEX_ERR_OOM; free(buf); free_token_list(out); return -1; 
    }

    if (out->count == 0) 
    {
        out->items = malloc(sizeof(char*));
        if (!out->items)
        {
            *errcode = LEX_ERR_OOM; free_token_list(out); return -1; 
        }
        out->items[0] = NULL;
    }

    return 0;
}
