#include "alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ALIASES 50

typedef struct 
{
    char *name;
    char *value;
} alias_t;

static alias_t aliases[MAX_ALIASES];

void alias_init()
{
    memset(aliases, 0, sizeof(aliases));
}

int alias_add(const char *name, const char *value)
{
    if (!name || !value) return -1;
    
    // Check if exists, update
    for (int i = 0; i < MAX_ALIASES; ++i)
    {
        if (aliases[i].name && strcmp(aliases[i].name, name) == 0)
        {
            free(aliases[i].value);
            aliases[i].value = strdup(value);
            return 0;
        }
    }

    // Add new
    for (int i = 0; i < MAX_ALIASES; ++i)
    {
        if (aliases[i].name == NULL)
        {
            aliases[i].name = strdup(name);
            aliases[i].value = strdup(value);
            return 0;
        }
    }
    
    fprintf(stderr, "foxy: too many aliases\n");
    return -1;
}

int alias_remove(const char *name)
{
    for (int i = 0; i < MAX_ALIASES; ++i)
    {
        if (aliases[i].name && strcmp(aliases[i].name, name) == 0)
        {
            free(aliases[i].name);
            free(aliases[i].value);
            aliases[i].name = NULL;
            aliases[i].value = NULL;
            return 0;
        }
    }
    return -1; 
}

const char *alias_resolve(const char *name)
{
    for (int i = 0; i < MAX_ALIASES; ++i)
    {
        if (aliases[i].name && strcmp(aliases[i].name, name) == 0)
        {
            return aliases[i].value;
        }
    }
    return NULL;
}

void alias_print_all()
{
    for (int i = 0; i < MAX_ALIASES; ++i)
    {
        if (aliases[i].name)
        {
            printf("%s='%s'\n", aliases[i].name, aliases[i].value);
        }
    }
}
