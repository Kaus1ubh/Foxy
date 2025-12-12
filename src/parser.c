#include "foxy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Parser helpers */
static node_t *new_node(node_type_t type)
{
    node_t *n = calloc(1, sizeof(node_t));
    if (!n) { fprintf(stderr, "foxy: OOM\n"); return NULL; }
    n->type = type;
    return n;
}

void free_ast(node_t *node)
{
    if (!node) return;
    if (node->type == NODE_CMD)
    {
        if (node->cmd.args)
        {
            for (int i = 0; node->cmd.args[i]; ++i) free(node->cmd.args[i]);
            free(node->cmd.args);
        }
        if (node->cmd.infile) free(node->cmd.infile);
        if (node->cmd.outfile) free(node->cmd.outfile);
    }
    else
    {
        free_ast(node->binary.left);
        free_ast(node->binary.right);
    }
    free(node);
}

/* 
 * Grammar:
 *  list     -> pipeline { (';' | '&' | '&&' | '||') pipeline }
 *  pipeline -> command { '|' command }
 *  command  -> WORD { WORD | REDIR }
 */



static int is_op(const char *s)
{
    return (strcmp(s, ";") == 0 || strcmp(s, "&&") == 0 || strcmp(s, "||") == 0 || strcmp(s, "|") == 0);
}

static node_t *parse_pipeline(char **tokens, int *pos, int count);

static node_t *parse_list(char **tokens, int *pos, int count)
{
    node_t *left = parse_pipeline(tokens, pos, count);
    if (!left) return NULL;

    while (*pos < count)
    {
        char *op = tokens[*pos];
        if (strcmp(op, ";") == 0)
        {
            (*pos)++;
            node_t *right = NULL;
            if (*pos < count) right = parse_list(tokens, pos, count);
            
            node_t *seq = new_node(NODE_SEQ);
            seq->binary.left = left;
            seq->binary.right = right;
            left = seq;
        }
        else if (strcmp(op, "&&") == 0)
        {
            (*pos)++;
            node_t *right = parse_pipeline(tokens, pos, count);
            if (!right) { fprintf(stderr, "foxy: syntax error near &&\n"); free_ast(left); return NULL; }
            
            node_t *and_n = new_node(NODE_AND);
            and_n->binary.left = left;
            and_n->binary.right = right;
            left = and_n;
        }
        else if (strcmp(op, "||") == 0)
        {
            (*pos)++;
            node_t *right = parse_pipeline(tokens, pos, count);
            if (!right) { fprintf(stderr, "foxy: syntax error near ||\n"); free_ast(left); return NULL; }
            
            node_t *or_n = new_node(NODE_OR);
            or_n->binary.left = left;
            or_n->binary.right = right;
            left = or_n;
        }
        else if (strcmp(op, "&") == 0) // Background at list level (or end of command)
        {
             (*pos)++;
             // For now, mark left as background. 
             // If left is a binary node, we might need to propagate or wrap.
             // Simplest: set flag on the node if possible or rely on top-level execution.
             // But existing struct has is_background in cmd and binary.
             if (left->type == NODE_CMD) left->cmd.bg_mode = 1;
             else left->binary.bg_mode = 1;
             
             // If there are more commands after &, treat as sequence? 

             // e.g. "sleep 1 & echo done" -> treated as sequence where left is bg.
             if (*pos < count)
             {
                 node_t *right = parse_list(tokens, pos, count);
                 node_t *seq = new_node(NODE_SEQ);
                 seq->binary.left = left;
                 seq->binary.right = right;
                 left = seq;
             }
        }
        else
        {
            break; // Not a list separator
        }
    }
    return left;
}

static node_t *parse_pipeline(char **tokens, int *pos, int count)
{
    // Parse command first
    // If next is |, consume and recurse
    
    // Command parsing: consume words until op or end
    int start = *pos;
    int end = start;
    
    while (end < count && !is_op(tokens[end]) && strcmp(tokens[end], "&") != 0 && strcmp(tokens[end], ")") != 0)
    {
        end++;
    }
    
    if (start == end) return NULL; // No command found?
    
    // Build CMD node
    node_t *cmd = new_node(NODE_CMD);
    
    // Pass 1: count args and handle redir
    int argc = 0;
    for (int i = start; i < end; ++i)
    {
        char *t = tokens[i];
        if (strcmp(t, "<") == 0)
        {
            if (i + 1 < end) { cmd->cmd.infile = strdup(tokens[++i]); }
            else { fprintf(stderr, "foxy: syntax error near <\n"); free_ast(cmd); *pos = end; return NULL; }
        }
        else if (strcmp(t, ">") == 0)
        {
            if (i + 1 < end) { cmd->cmd.outfile = strdup(tokens[++i]); cmd->cmd.append_out = 0; }
            else { fprintf(stderr, "foxy: syntax error near >\n"); free_ast(cmd); *pos = end; return NULL; }
        }
        else if (strcmp(t, ">>") == 0)
        {
            if (i + 1 < end) { cmd->cmd.outfile = strdup(tokens[++i]); cmd->cmd.append_out = 1; }
            else { fprintf(stderr, "foxy: syntax error near >>\n"); free_ast(cmd); *pos = end; return NULL; }
        }
        else
        {
            argc++;
        }
    }
    
    cmd->cmd.args = malloc(sizeof(char*) * (argc + 1));
    int ai = 0;
    for (int i = start; i < end; ++i)
    {
        char *t = tokens[i];
        if (strcmp(t, "<") == 0 || strcmp(t, ">") == 0 || strcmp(t, ">>") == 0) { i++; continue; }
        cmd->cmd.args[ai++] = strdup(t);
    }
    cmd->cmd.args[argc] = NULL;
    
    *pos = end;
    
    // Check for Pipe
    if (*pos < count && strcmp(tokens[*pos], "|") == 0)
    {
        (*pos)++;
        node_t *right = parse_pipeline(tokens, pos, count);
        if (!right) { fprintf(stderr, "foxy: syntax error near |\n"); free_ast(cmd); return NULL; }
        
        node_t *pipe = new_node(NODE_PIPE);
        pipe->binary.left = cmd;
        pipe->binary.right = right;
        return pipe;
    }
    
    return cmd;
}

node_t *parse_tokens(token_list_t *tokens)
{
    if (!tokens || !tokens->items || tokens->count == 0) return NULL;
    int pos = 0;
    return parse_list(tokens->items, &pos, tokens->count);
}
