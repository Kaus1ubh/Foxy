#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_JOBS 20

static job_t job_list[MAX_JOBS];
static int next_job_id = 1;

void job_init()
{
    memset(job_list, 0, sizeof(job_list));
    next_job_id = 1;
}

int job_add(intptr_t pid, const char *command)
{
    for (int i = 0; i < MAX_JOBS; ++i)
    {
        if (job_list[i].id == 0) // Empty slot
        {
            job_list[i].id = next_job_id++;
            job_list[i].pid = pid;
            job_list[i].command = strdup(command);
            job_list[i].status = JOB_RUNNING;
            
            // Print background job info: [1] 1234
            printf("[%d] %lld\n", job_list[i].id, (long long)pid);
            return job_list[i].id;
        }
    }
    fprintf(stderr, "foxy: too many jobs\n");
    return -1;
}

job_t *job_find(int id)
{
    for (int i = 0; i < MAX_JOBS; ++i)
    {
        if (job_list[i].id == id && job_list[i].status == JOB_RUNNING)
        {
            return &job_list[i];
        }
    }
    return NULL;
}

void job_print_all()
{
    for (int i = 0; i < MAX_JOBS; ++i)
    {
        if (job_list[i].id != 0)
        {
            printf("[%d] %s %s\n", 
                job_list[i].id, 
                job_list[i].status == JOB_RUNNING ? "Running" : "Done",
                job_list[i].command);
        }
    }
}

void remove_job(int index)
{
    if (job_list[index].command) free(job_list[index].command);
    job_list[index].command = NULL;
    job_list[index].id = 0;
    job_list[index].pid = 0;
    job_list[index].status = JOB_DONE;
}

void job_check_status()
{
    for (int i = 0; i < MAX_JOBS; ++i)
    {
        if (job_list[i].id != 0 && job_list[i].status == JOB_RUNNING)
        {
            DWORD exit_code;
            HANDLE hProcess = (HANDLE)job_list[i].pid;
            
            if (GetExitCodeProcess(hProcess, &exit_code))
            {
                if (exit_code != STILL_ACTIVE)
                {
                    // Job finished
                    printf("[%d] Done %s\n", job_list[i].id, job_list[i].command);
                    job_list[i].status = JOB_DONE;
                    // We can remove it now, or keep it until user sees it? 
                    // Standard shell: Prints on next prompt.
                    // For simplicity, remove it after printing.
                    // CloseHandle needed? The CRT _spawnvp might own it?
                    // Actually _spawnvp returns a handle but documentation says we should wait on it or close it.
                    // Since we didn't wait, we might need to CloseHandle if we are done with it.
                    // But typically _P_NOWAIT means caller must handle it.
                    CloseHandle(hProcess);
                    remove_job(i);
                }
            }
        }
    }
}

int job_to_foreground(int id)
{
    job_t *j = job_find(id);
    if (!j)
    {
        fprintf(stderr, "foxy: job %d not found\n", id);
        return -1;
    }

    HANDLE hProcess = (HANDLE)j->pid;
    // Wait for it
    WaitForSingleObject(hProcess, INFINITE);
    
    // It's done
    job_check_status(); // This will cleanup and print "Done"
    return 0;
}
