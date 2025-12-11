#ifndef FOXY_H
#define FOXY_H

#include <stddef.h>

typedef enum { LEX_OK = 0, LEX_ERR_UNCLOSED_QUOTE = 1, LEX_ERR_OOM = 2 } lex_err_t;

typedef struct 
{
    char **items;
    size_t count;

} token_list_t;

/* Lexer API */
int tokenize_line(const char *line, token_list_t *out, lex_err_t *errcode);
void free_token_list(token_list_t *t);

int builtin_dispatch(char **tokens);

#endif // FOXY_H
