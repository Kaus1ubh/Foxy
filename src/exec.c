#include "foxy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#ifdef _WIN32
#include <process.h>
#include <windows.h>
#include <io.h>
#define pipe(fds) _pipe(fds, 4096, _O_BINARY)
#define dup2 _dup2
#define dup _dup
#define close _close
#define fileno _fileno
#define execvp _execvp
#define WIFEXITED(x) 1
#define WEXITSTATUS(x) (x)
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

int builtin_dispatch(char **tokens);

#ifdef _WIN32
// Windows-specific spawn helper
static int spawn_command(node_t *node, int input_fd, int output_fd)
{
    // Save standard FDs
    int saved_stdin = -1;
    int saved_stdout = -1;
    
    if (input_fd != -1)
    {
        saved_stdin = dup(0);
        dup2(input_fd, 0);
    }
    else if (node->cmd.infile)
    {
        saved_stdin = dup(0);
        int fd = open(node->cmd.infile, O_RDONLY);
        if (fd < 0) { perror(node->cmd.infile); return 1; }
        dup2(fd, 0);
        close(fd);
    }

    if (output_fd != -1)
    {
        saved_stdout = dup(1);
        dup2(output_fd, 1);
    }
    else if (node->cmd.outfile)
    {
        saved_stdout = dup(1);
        int flags = O_WRONLY | O_CREAT;
        if (node->cmd.append_out) flags |= O_APPEND;
        else flags |= O_TRUNC;
        int fd = open(node->cmd.outfile, flags, 0644);
        if (fd < 0) { perror(node->cmd.outfile); return 1; }
        dup2(fd, 1);
        close(fd);
    }

    // Check builtin
    char **argv = node->cmd.args;
    // Force NOWAIT if background flag is set
    int mode = node->cmd.is_background ? _P_NOWAIT : _P_WAIT;
    int status = 0;

    if (builtin_dispatch(argv))
    {
        status = 0; 
    }
    else
    {
        // Not a builtin, spawn
        int ret = _spawnvp(mode, argv[0], (const char * const *)argv);
        if (ret == -1)
        {
            perror("foxy: spawn");
            status = 127;
        }
        else
        {
            if (mode == _P_NOWAIT)
            {
                 // ret is HANDLE (or pid cast to intptr_t cast to int)
                 // returning 0 to executor to signify "started bg"
                 status = 0; 
            }
            else
            {
                status = ret;
            }
        }
    }

    // Restore Standard FDs
    if (saved_stdin != -1)
    {
        dup2(saved_stdin, 0);
        close(saved_stdin);
    }
    if (saved_stdout != -1)
    {
        dup2(saved_stdout, 1);
        close(saved_stdout);
    }

    return status;
}
#endif

int exec_node(node_t *node)
{
    if (!node) return 0;
    
    switch (node->type)
    {
        case NODE_CMD:
#ifdef _WIN32
            return spawn_command(node, -1, -1);
#else
            return 0; 
#endif
            
        case NODE_SEQ:
            exec_node(node->binary.left);
            return exec_node(node->binary.right);
            
        case NODE_AND:
        {
            int status = exec_node(node->binary.left);
            if (status == 0) return exec_node(node->binary.right);
            return status;
        }
        
        case NODE_OR:
        {
            int status = exec_node(node->binary.left);
            if (status != 0) return exec_node(node->binary.right);
            return status;
        }
        
        case NODE_PIPE:
        {
#ifdef _WIN32
            int pfds[2];
            if (_pipe(pfds, 4096, _O_BINARY) == -1) { perror("pipe"); return 1; }
            
            // Left command async
            int saved_stdout = dup(1);
            int saved_stdin = dup(0);
            
            dup2(pfds[1], 1);
            close(pfds[1]); 

            // Hack: Mark left as background to force _P_NOWAIT
            // Accessing union members: depends on type
            if (node->binary.left->type == NODE_CMD) node->binary.left->cmd.is_background = 1;
            else node->binary.left->binary.is_background = 1;

            exec_node(node->binary.left); // Ignoring handle
            
            dup2(saved_stdout, 1);
            close(saved_stdout);
            
            dup2(pfds[0], 0);
            close(pfds[0]);
            
            int stat2 = exec_node(node->binary.right); // Right runs sync
            
            dup2(saved_stdin, 0);
            close(saved_stdin);
            
            return stat2;
#else
            return 1;
#endif
        }
        
        default:
            return 1;
    }
}
