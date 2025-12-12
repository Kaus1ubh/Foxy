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

/* AST */
typedef enum 
{ 
    NODE_CMD, 
    NODE_PIPE, 
    NODE_SEQ,      // ;
    NODE_AND,      // &&
    NODE_OR,       // ||
} node_type_t;

typedef struct node_t 
{
    node_type_t type;
    union 
    {
        struct 
        {
            char **args;       // argv, NULL terminated
            char *infile;      // <
            char *outfile;     // > or >>
            int append_out;    // 1 if >>, 0 if >
            int is_background; // &
        } cmd;

        struct 
        {
            struct node_t *left;
            struct node_t *right;
            int is_background; // for pipelines/logic?
        } binary;
    };
} node_t;

/* Parser API */
node_t *parse_tokens(token_list_t *tokens);
void free_ast(node_t *node);

/* Executor API */
int exec_node(node_t *node);

/* Prompt API */
void set_prompt_format(const char *fmt);


#endif // FOXY_H
