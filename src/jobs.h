#ifndef JOBS_H
#define JOBS_H

#include <stddef.h>
#include <stdint.h>

typedef enum 
{ 
    JOB_RUNNING, 
    JOB_DONE 
} job_status_t;

typedef struct 
{
    int id;             // Job ID (1, 2, ...)
    intptr_t pid;       // Process Handle/ID (Windows _spawnvp returns intptr_t)
    char *command;      // Command string
    job_status_t status;
} job_t;

void job_init();
int job_add(intptr_t pid, const char *command);
void job_print_all();
job_t *job_find(int id);
void job_check_status(); // Checks for finished background jobs and prints notification
int job_to_foreground(int id); // Waits for job

#endif // JOBS_H
