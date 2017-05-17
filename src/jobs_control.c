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

static void _new_process(Process ** p) {
    *p = (Process *) malloc(sizeof (Process));
    (*p)->next = NULL;
    (*p)->argc = 0;
    (*p)->state = READY;
}

static void prepare_job(Job * job) {
    int i = 0;
    Process ** proc = &(job->proc);
    const char * ptr = job->command;
    int offset = 0;
    char del = ' ';

    _new_process(proc);
    
    while (ptr[offset] != '\0' && ptr[offset] != '&' && ptr[offset] != '+'  &&
           (*proc)->argc < MAX_LINE_COMMAND) {
        
        // Se procesa el caracter leido.
        if (*ptr == ' ') {
            ptr++;
        } else if (*ptr == '|') { // Siguiente proceso...
            proc = &((*proc)->next);
            _new_process(proc);
            i = 0;
            ptr++;
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

    if (*ptr == '+') {
        job->respawnable = 1;
        job->foreground = 0;
    }
    else if (*ptr == '&')
        job->foreground = 0;
    else if (*ptr != '\0' && offset != 0 && (*proc)->argc < MAX_ARGS) { // Si había algo que copiar; se hace.
        allocateAndCopy(&(*proc)->args[i], ptr, offset);
        i++;
        ((*proc)->argc)++;
    }
    
    // marca el fin del comando.
    (*proc)->args[i] = NULL;
    job->info = &((*proc)->info);
}

void init_list_jobs(ListJobs * list_jobs) {
    *list_jobs = NULL;
}

void destroy_processes(Job * job, int n) {
    Process * curr, *prev = NULL, *rmNode = NULL;
    char ** arg;
    
    curr = job->proc;
    
    while (curr) {
        
        arg = curr->args;
        
        if (n == -1 || n == curr->num_job ) {
            
            while(*arg) {
                free(*arg);
                arg++;
            }
            
            if (prev)
                prev->next = curr->next;
            else
                job->proc = job->proc->next;
            
            rmNode = curr;
        }
        else 
            prev = curr;            

        curr = curr->next;
        
        if ( rmNode) {
            free(rmNode);
            rmNode = NULL;
        }
        
    }
    
    // significa que se ha borrado todos los procesos
    if ( !prev )
        job->proc = NULL;
    
}

void destroy_list_jobs(ListJobs * list_jobs) {
    Job * curr = *list_jobs;
    Job * prev;
    
    while (curr) {
        prev = curr;
        destroy_processes(curr, -1);
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
    (*curr)->status = READY;
    (*curr)->info = 0;
    (*curr)->next = NULL;
    (*curr)->cargarModo = 0;
    (*curr)->notify = 0;
    (*curr)->total = 1;
    (*curr)->active = 0;
    (*curr)->type = NORMAL_JOB;
    (*curr)->respawnable = 0;
    (*curr)->time_out = 0;
    prepare_job(*curr);

    return *curr;
}

void remove_job_n(ListJobs * jobs, pid_t gpid, int n) {
    Job * curr = *jobs, *prev = NULL;
    Process * p;
    
    // Busco el trabajo...
    while ( curr && curr->gpid != gpid ) {
        prev = curr;
        curr = curr->next;
    }
    
    // lo borro si existe...
    if ( curr ) {
        destroy_processes(curr, n);
        
        if (curr->proc) // se ha borrado 1.
            curr->total--;
        else { // borrado del nodo.
            
            if ( !prev )
                *jobs = (*jobs)->next;
            else
                prev->next = curr->next;
            
            free(curr);
            curr = NULL;
        }
        
    }
    
}

void reenumerate_job(Job * job) {
    Process * p = job->proc, * prev = NULL;
    int num = 0, anterior;
    
    if (!job->proc)
        return;
    
    anterior = p->num_job;
    while (p) {
        
        if (anterior != p->num_job) {
            anterior = p->num_job;
            num++;
        }
        
        p->num_job = num;
        p = p->next;
    }
    
}

void dup_job_command(Job * job) {
    Process ** dst = &(job->proc); // Apunta al puntero escritor.
    Process ** src = &(job->proc); // Apunta al puntero lector.
    int i;
    
    if (*dst) {
        // vamos al final.
        while (*dst) 
            dst = &( (*dst)->next );
        // Comenzamos a copiar con numero = job->total
        while (*src && (*src)->num_job == 0) {
            i = 0;
            *dst = (Process *) malloc(sizeof(Process));
            // Copiamos todos los argumentos.
            while ( (*src)->args[i] ) {
                (*dst)->args[i] = (char *) malloc(sizeof(char) * strlen((*src)->args[i]));
                strcpy( (*dst)->args[i], (*src)->args[i]);
                i++;
            }
            (*dst)->args[i] = NULL;
            (*dst)->argc = (*src)->argc;
            // Especificamos lo que queda.
            (*dst)->state = READY; 
            (*dst)->num_job = job->total;
            (*dst)->next = NULL;
            // Cogemos la dirección del siguiente e iteramos.
            dst = &( (*dst)->next );
            src = &( (*src)->next );
        }
        
        job->total++;
    }
    
}

char is_job_n_running(Job * job, int i) {
    char running = 0;
    Process * p = job->proc;
    
    while (p && !running) {
        
        if (p->num_job == i || i == -1)
            running = p->state == RUNNING;
        
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
            finish = finish && (p->state == COMPLETED || p->state == SIGNALED);
            
            if (p->state == SIGNALED && signaled != NULL && !(*signaled) ) {
                *signaled = 1;
                job->info = &(p->info);
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
            
            if (p->state == RUNNING)
                nRunning++;
            else if (p->state == STOPPED)
                nStopped++;
        
        p = p->next;
    }
    
    return nStopped > 0 && nRunning == 0;
}

static void next_proc_state(Process * p, int status) {
    
    if (p->state == READY)
        p->state = RUNNING;
    else if ( (p->state == STOPPED && WIFCONTINUED(status)))
        p->state = RUNNING;
    else if ( (p->state == RUNNING && WIFSTOPPED(status))) {
        p->state = STOPPED;
        p->info = WSTOPSIG(status);
    }
    else if ( (p->state == STOPPED || p->state == RUNNING) &&
            WIFEXITED(status) ) 
    {
        p->state = COMPLETED;
        p->info = WEXITSTATUS(status);
    }
    else if ( (p->state == STOPPED || p->state == RUNNING) &&
            WIFSIGNALED(status) ) 
    {
        p->state = SIGNALED;
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
    Process * p = job->proc;
    
    if (is_job_completed(job, &signaled)) {
        
        if (signaled) 
            job->status = SIGNALED;
        else 
            job->status = COMPLETED;
        
    }
    else if (is_job_stopped(job)) {
        job->status = STOPPED;
    }
    else {
        job->status = RUNNING;
    }
    
}

Job * search_job_by_process(ListJobs jobs, pid_t pid) {
    Job * j = NULL;
    Job * curr = jobs;
    Process * p;
    
    while (curr && !j) {
        p = curr->proc;
        
        while (p && !j) {
            
            if (p->pid == pid)
                j = curr;
            
            p = p->next;
        }
        
        curr = curr->next;
    }
    
    return j;
}

void kill_job(Job * job, int n, int sig) {
    Process * p = job->proc;
    
    while (p) {
        
        if (p->num_job == n)
            kill(p->pid, sig);
        
        p = p->next;
    }
    
}
