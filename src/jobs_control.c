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
    (*proc)->state = P_READY;

    while (ptr[offset] != '\0' && ptr[offset] != '&' && (*proc)->argc < MAX_LINE_COMMAND) {
        
        // Se procesa el caracter leido.
        if (*ptr == ' ') {
            ptr++;
        } else if (*ptr == '|') { // Siguiente proceso...
            proc = &((*proc)->next);
            *proc = (Process *) malloc(sizeof (Process));
            (*proc)->next = NULL;
            (*proc)->state = P_READY;
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
    (*curr)->status = JOB_READY;
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

char is_job_n_running(Job * job, int i) {
    char running = 0;
    Process * p = job->proc;
    
    while (p && !running) {
        
        if (p->num_job == i || i == -1)
            running = p->state == P_RUNNING;
        
        p = p->next;
    }
    
    return running;
}

char is_job_n_completed(Job * job, int i, char * signaled) {
    char finish = 1;
    Process * p = job->proc;
    
    if (signaled != NULL)
        *signaled = 0;
    
    while (p && finish) {
        
        if (p->num_job == i || i == -1) {
            finish = finish && (p->state == P_EXITED || p->state == P_SIGNALED);
            
            if (p->state == P_SIGNALED && signaled != NULL && !(*signaled) ) {
                *signaled = 1;
                job->info = p->info;
            }
            
        }
        
        p = p->next;
    }
    
    return finish;
}

char is_job_n_stopped(Job * job, int i) {
    Process * p = job->proc;
    int nRunning = 0;
    int nStopped = 0;
    
    while (p ) {
        
        if (p->num_job == i || i == -1) 
            
            if (p->state == P_RUNNING)
                nRunning++;
            else if (p->state == P_STOPPED)
                nStopped++;
        
        p = p->next;
    }
    
    return nStopped > 0 && nRunning == 0;
}


void next_proc_state(Process * p, int status) {
    
    if (p->state == P_READY)
        p->state = P_RUNNING;
    else if ( (p->state == P_STOPPED && WIFCONTINUED(status)))
        p->state = P_RUNNING;
    else if ( (p->state == P_RUNNING && WIFSTOPPED(status))) {
        p->state = P_STOPPED;
        p->info = WSTOPSIG(status);
    }
    else if ( (p->state == P_STOPPED || p->state == P_RUNNING) &&
            WIFEXITED(status) ) 
    {
        p->state = P_EXITED;
        p->info = WEXITSTATUS(status);
    }
    else if ( (p->state == P_STOPPED || p->state == P_RUNNING) &&
            WIFSIGNALED(status) ) 
    {
        p->state = P_SIGNALED;
        p->info = WTERMSIG(status);
    }
}

void mark_process(Job * job, int status, pid_t pid) {
    Process * p = job->proc;
    char founded = 0;
    
    while (p && !founded) {
        
        if (p->pid == pid) {
            next_proc_state(p, status);
            founded = 1;
        }
        
        p = p->next;
    }
    
}

void analyce_job_status(Job * job) {
    char signaled;
    
    if (is_job_completed(job, &signaled)) {
        
        if (signaled) 
            job->status = JOB_SIGNALED;
        else
            job->status = JOB_COMPLETED;
        
    }
    else if (is_job_stopped(job)) {
        job->status = JOB_STOPPED;
    }
    else {
        job->status = JOB_RUNNING;
    }
    
}