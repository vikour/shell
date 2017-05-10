/**
 * Implementación del control de trabajos..
 * 
 * @file  jobs_control.c
 * @autor Víctor Manuel Ortiz Guardeño.
 * @date  27/04/2017
 */

#include <jobs_control.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


static void allocateAndCopy(char ** dest, const char * orig, int count) {
    *dest = (char*) malloc(sizeof (char) *(count + 1));
    strncpy(*dest, orig, count);
    (*dest)[count] = '\0';
    *(dest + 1) = NULL;
}

static void prepare_job(Job * job) {
    int i = 0;
    Process ** proc = &(job->proc);
    const char * ptr = job->command;
    int offset = 0;
    char del = ' ';

    *proc = (Process *) malloc(sizeof (Process));
    (*proc)->next = NULL;
    (*proc)->argc = 0;

    while (ptr[offset] != '\0' && ptr[offset] != '&' && (*proc)->argc < MAX_LINE_COMMAND) {
        
        // Se procesa el caracter leido.
        if (*ptr == ' ') {
            ptr++;
        } else if (*ptr == '|') { // Siguiente proceso...
            proc = &((*proc)->next);
            *proc = (Process *) malloc(sizeof (Process));
            (*proc)->next = NULL;
            i = 0;
            ptr++;
            ((*proc)->argc)++;
        } else if ((*ptr == '\'' || *ptr == '\"') && offset <= 1) { // cambio de delimitador.
            del = *ptr;
            offset++;
        } else if (*(ptr + offset) == del) {

            if (*(ptr + offset) != ' ') // Si el delimitador es distinto de espacio, lo copiamos.
                allocateAndCopy(&(*proc)->args[i], ptr, offset + 1);
            else // Si no, no cogemos el espacio.
                allocateAndCopy(&(*proc)->args[i], ptr, offset);

            i++;
            ptr += offset + 1; // saltamos ese espacio.
            offset = 0;
            del = ' ';
            ((*proc)->argc)++;
        } else {
            offset++;
        }

    } // end while.

    if (*ptr == '&')
        job->foreground = 0;
    else if (*ptr != '\0' && offset != 0 && (*proc)->argc < MAX_ARGS) { // Si había algo que copiar; se hace.
        allocateAndCopy(&(*proc)->args[i], ptr, offset);
        i++;
        ((*proc)->argc)++;
    }
    
    // marca el fin del comando.
    (*proc)->args[i] = NULL;

}

void init_list_jobs(ListJobs * list_jobs) {
    *list_jobs = NULL;
}

void destroy_processes(Job * job) {
    Process * curr, *prev;
    char ** arg;
    
    curr = job->proc;
    
    while (curr) {
        arg = curr->args;
        prev = curr;
        
        while(*arg) {
            free(*arg);
            arg++;
        }
        
        curr = curr->next;
        free(prev);
    }
    
}

void destroy_list_jobs(ListJobs * list_jobs) {
    Job * curr = *list_jobs;
    Job * prev;
    
    while (curr) {
        prev = curr;
        destroy_processes(curr);
        curr = curr->next;
        free(prev);
    }
    
}

Job * create_job(ListJobs * list_jobs, const char * cmd) {
    Job ** curr = list_jobs;

    if (list_jobs == NULL || cmd == NULL)
        return NULL;
    
    while (*curr)
        curr = &((*curr)->next);

    *curr = (Job *) malloc(sizeof (Job));
    (*curr)->command = cmd;
    (*curr)->foreground = 1;
    (*curr)->gpid = 0;
    (*curr)->status = job_ready;
    (*curr)->info = 0;
    (*curr)->next = NULL;
    (*curr)->cargarModo = 0;
    (*curr)->notify = 0;
    prepare_job(*curr);

    return *curr;
}

void remove_job(ListJobs * list_jobs, pid_t gpid) {
    Job * prev, * curr;
    
    prev = NULL;
    curr = *list_jobs;
    
    while (curr && curr->gpid != gpid) {
        prev = curr;
        curr = curr->next;
    }
    
    if (prev == NULL) 
        *list_jobs = (*list_jobs)->next;
    else if (curr)
        prev->next = curr->next;
    
    if (curr) {
        destroy_processes(curr);
        free(curr);
    }
    
}

void next_state(Job * job, int status, char foreground, char exec) {
    
    // Job ejecución foreground:
    if ( (job->status == job_ready && exec == 1 && job->foreground) ||
         (job->status == job_stopped && WIFCONTINUED(status) && foreground) ||
         (job->status == job_executed && !job->foreground && foreground))
    {
        job->status = job_executed;
        job->foreground = 1;
    }
    // Job ejecución background
    else if ( (job->status == job_ready && exec == 1 && !job->foreground) ||
              (job->status == job_stopped && WIFCONTINUED(status) && !foreground))
    {
        job->status = job_executed;
        job->foreground = 0;
    }
    // Job detenido
    else if ( (job->status == job_executed && WIFSTOPPED(status))) {
        job->status = job_stopped;
        job->foreground = 0;
        job->info = WSTOPSIG(status);
    }
    // Job completed
    else if ( job->status == job_executed && WIFEXITED(status) ) {
        job->status = job_completed;
        job->info = WEXITSTATUS(status);
    }
    // job signaled
    else if ( (job->status == job_executed || job->status == job_stopped) && WIFSIGNALED(status) ) 
    {
        job->status = job_signaled;
        job->info = WTERMSIG(status);
    }
    
}