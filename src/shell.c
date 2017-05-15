/**
 * Implementación de la shell.
 * 
 * @file  shell.c
 * @autor Víctor Manuel Ortiz Guardeño
 * @date  27/04/2017
 */

#include <shell.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <wait.h>
#include <sysexits.h>

void control_signals(void (*handler)(int)) {
    signal(SIGQUIT, handler);
    signal(SIGINT,  handler);
    signal(SIGTTIN, handler);
    signal(SIGTTOU, handler);
    signal(SIGTERM, handler);
    signal(SIGTSTP, handler);
}

void print_job_state(int number, Job * job) {
    
    printf("[%d]\t", number);

    switch (job->status) {

        case COMPLETED:
            printf("%-15s","Hecho");
            break;

        case STOPPED:
            printf("%-15s","Detenido");
            break;

        case RUNNING:
            printf("%-15s","En ejecución");
            break;

        case SIGNALED:
            printf("%-15s","Signaled");
            break;

    }
    
    printf("\t%s ", job->command);
    
    if (job->total > 1)
        printf("{*%d}", job->total);
    
    putchar('\n');
}


void block_sig(int sig) {
    sigset_t block_sigchld;
    
    sigemptyset(&block_sigchld);
    sigaddset(&block_sigchld, sig);
    sigprocmask(SIG_BLOCK, &block_sigchld, NULL);
}

void unblock_sig(int sig) {
    sigset_t block_sigchld;
    
    sigemptyset(&block_sigchld);
    sigaddset(&block_sigchld, sig);
    sigprocmask(SIG_UNBLOCK, &block_sigchld, NULL);
}

/**
 * Elimina todos los trabajos internos de un grupo de trabajos que ya han terminado.
 * 
 * @param j  Trabajo del que se quieren eliminar los trabajos.
 */

void cleanInnerJobs(Job * j) {
    int total, i;
    total = j->total;
    
    block_sig(SIGALRM);
    // Actualizamos los trabajos internos si hay más de uno.
    for (i = 0; i < j->total && j->total > 1; i++)

        if (is_job_n_completed(j, i, NULL))
            remove_job_n(&shell.jobs, j->gpid, i);

    if (total != j->total)
        reenumerate_job(j);
    unblock_sig(SIGALRM);
}

void roundRobin(int sig) {
    Job * j = shell.jobs;
    int current, next, updated = 0;
    
    while (j) {
        
        if (j->type == RR_JOB) 
            
            if (j->total > 1) {
                current = j->active;
                next = (current + 1) % j->total;
                j->active = next;
                kill_job(j,current,SIGSTOP);
                kill_job(j,next,SIGCONT);
                updated++;
            }
            // Si sólo hay uno, puede que esté parado, porque no le tocaba. Así
            // que lo pongo en marcha. Esto puede suceder, cuando se killall al
            // comando round robin, y el último está parado, pero tiene planificada
            // la señal de terminar.
            else
                kill(-j->gpid, SIGCONT);

        
        j = j->next;
    }
    
    if (updated)
        alarm(1);
    else
        shell.sigalarm_on = 0;
}

/**
 * El manejador de SIGCHLD actualiza la lista de trabajos.
 */

void updateJobs(int sig) {
    Job * j = shell.jobs;
    int status, total, i;
    pid_t pid;
    Process * p;

    while (j) {
        p = j->proc;
        
        while (p) {

            pid = waitpid(p->pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
            
            if (pid > 0) 
                mark_process(j, status, pid);
            
            p = p->next;
        }
        
        if (j->total > 1)
            cleanInnerJobs(j);
        
        analyce_job_status(j);
        j->notify = (j->status == STOPPED || IS_JOB_ENDED(j->status)) && !j->foreground;
        j = j->next;
        
    }
    
}

void init_shell() {
    shell.fdin = fileno(stdin);
    shell.pid = getpid();

    // Comprobamos que podemos usar la entrada estándar como
    // una terminal.
    if (!isatty(shell.fdin)) {
        perror("isatty(shell.fdin)");
        exit(-1);
    }

    // establecemos el grupo para la terminal.
    setpgid(shell.pid, shell.pid);

    // Obtenemos el control de la terminal.
    tcsetpgrp(shell.fdin, shell.pid);

    // creamos el historial.
    initHist(&(shell.hist));
    
    // creamos la lista de trabajos.
    init_list_jobs(&shell.jobs);

    // Obtengo las opciones actuales de la terminal.
    tcgetattr(shell.fdin, &(shell.mode));
    
    // Nos ponemos en nuestro propio grupo.
    if ( setpgid(shell.pid, shell.pid) < 0) {
        perror("setpgid");
        exit(-1);
    }
    
    // Ignoramos todo.
    control_signals(SIG_IGN);
    
    // Manejamos la señal de SIGALARM.    
    signal(SIGALRM, roundRobin);
    shell.sigalarm_on = 0;
    
    // Manejamos la señal SIGCHLD
    signal(SIGCHLD, updateJobs);
}

void destroy_shell() {
    destroyHist(&(shell.hist));
    destroy_list_jobs(&shell.jobs);
}

void report_job_foreground(Job * job) {
    char signaled;
    
    print_info("Foreground job ... pid : %d, command : %s, ", job->gpid, job->command);
    
    if (job->status == STOPPED) {
        tcgetattr(shell.fdin, &job->tmodes);
        job->cargarModo = 1;
        job->foreground = 0;
        print_info("detenido\n");
    }
    else {
        
        if (job->status == COMPLETED) {
            print_info("exited : %d\n", *(job->info));
        }
        else  {
            print_info("signaled : %d\n", *(job->info));
        }
        
        remove_job(&shell.jobs, job->gpid);
    }
}

void put_job_foreground(Job * job) {
    int status;
    pid_t pid;
    
    // Pospongo las señales del manejador, hasta que este proceso se maneje.
    block_sig(SIGCHLD);
    
    // Si se almacenó el modo en el que el comando se detuvo, se reestablece.
    if ( job->cargarModo ) {
        tcsetattr(shell.fdin, TCSADRAIN, &job->tmodes);
    }
    
    job->foreground = 1;
    tcsetpgrp(shell.fdin, job->gpid);
    
    // Si el trabajo se paró..
    if ( job->status == STOPPED) 
        kill(-job->gpid, SIGCONT);    
    
    do {
        pid = waitpid(- job->gpid, &status, WUNTRACED);
        
        if (pid > 0) {
            mark_process(job, status, pid);
            analyce_job_status(job);
        }
        
    } while ( job->status == RUNNING );
    
    report_job_foreground(job);
    unblock_sig(SIGCHLD);
    
    tcsetpgrp(shell.fdin, shell.pid);
    tcsetattr(shell.fdin, TCSANOW,&shell.mode);
}

void put_job_background(Job * job) {
    
    if (job->status == STOPPED) {
        job->status = RUNNING;
        job->foreground = 0;
        kill(- job->gpid, SIGCONT);
    }
    
    print_info("Background job ... pid : %d, command : %s\n", job->gpid, job->command);
}

void launch_process(Process * p, int fdin, int fdout, pid_t gpid, char foreground) {
    pid_t pid;
    int icmd;
    
    pid = getpid();
    
    if (gpid == 0)
        gpid = pid;
    
    setpgid(pid, gpid);

    if (foreground)
        tcsetpgrp(shell.fdin, gpid);
    
    signal(SIGCHLD, SIG_DFL);
    control_signals(SIG_DFL);

    icmd = indexOfInternalProcess(p);
    // Si no es un comando interno, o este no tiene manejador
    if (icmd < 0 || !ICMD_HANDLER(icmd)) { 
        execvp(p->args[0], p->args);
        putchar('\n');
        print_error("Error comando no enonctrado, commando : %s\n", p->args[0]);
        exit(errno);
    } // Si es interno, se ejecuta el manejador.
    else {
        ICMD_HANDLER(icmd)(p);
        exit(0);
    }
    
}

void launch_forked_job(Job * job) {
    Process * p = job->proc;
    
    while (p) {
        p->pid = fork(); 
        
        if (p->pid == 0) 
            launch_process(p, shell.fdin, STDOUT_FILENO,job->gpid,job->foreground);
        else {
            
            if (job->gpid == 0)
                job->gpid = p->pid;
            
            setpgid(p->pid, job->gpid);
            mark_process(job,0,p->pid);
        }
        
        p = p->next;
    }
    
    if (job->foreground)
        put_job_foreground(job);
    else
        put_job_background(job);
    
}

/**
 * Permite conocer si un trabajo es interno o no. Si no lo es, devolverá un valor menor
 * que 0. En caso de que sea, devolverá su índice.
 * 
 * @param job  Trabajo.
 * @return  índice del trabajo interno, o -1 en caso de que no sea interno.
 */

int indexOfInternalProcess(Process * p) {
    int index = -1;
    int j = 0;
    
    while (index == -1 && j < ICMD_TOTAL) {
        
        if (strcmp(p->args[0], ICMD_STR(j)) == 0)
            index = j;
        
        j++;
    }
    
    
    return index;
}

void launch_job(Job * job) {
    int index;
    
    if (job == NULL) // Causado por una línea vacía por el 
        return;
    
    index = indexOfInternalProcess(job->proc);
    
    if (index >= 0 && ICMD_HANDLER(index) && !ICMD_FORK(index)) {
        job->gpid = -1;
        internalCommands.handler[index](job->proc);
        block_sig(SIGCHLD);
        if (index ==  cmd_rr) remove_job(&shell.jobs, -1);
        unblock_sig(SIGCHLD);
    }
    else
        launch_forked_job(job);
    
}

// ---------------------------------------------------------------------------//
// -------------------------- COMANDOS INTERNOS-------------------------------//
// ---------------------------------------------------------------------------//

void cmd_rr_handler(Process * p) {
    Job * job = search_job_by_process(shell.jobs, p->pid);
    int num, i;
    
    if (p->argc < 3) {
        print_error("Formato: rr <num> <command>\n");
        return;
    }
    
    num = atoi(p->args[1]);
    
    if (num < 1) {
        print_error("El número debe ser mayor que 1\n");
        return;
    }
    
    // Eliminamos del proceso rr y el número.
    for (i = 0 ; i < p->argc - 1; i++)
        p->args[i] = p->args[i+2];
    
    p->argc -= 2;
    job->foreground = 0;
    job->type = RR_JOB;
    // Duplicamos los trabajos.
    for (i = 0 ; i < num - 1 ; i++)
        dup_job_command(job);
    
    while (p) {
        p->pid = fork();
        
        if (!p->pid)
            launch_process(p,shell.fdin,STDOUT_FILENO,job->gpid,job->foreground);
        // padre.
        
        if (job->gpid == -1)
            job->gpid = p->pid;
        
        setpgid(p->pid,job->gpid);
        mark_process(job,0,p->pid);
        p = p->next;
    }
    
    kill(-job->gpid, SIGSTOP);
    kill_job(job, 0, SIGCONT);
    analyce_job_status(job);
    //job->activo = job->total - 1;
    print_info("Background job ... pid : %d, command : %s\n", job->gpid,
               job->command);
    
    if (!shell.sigalarm_on) {
        alarm(1);
        shell.sigalarm_on = 1;
    }
}

void cmd_cd_handler(Process * p) {
    const char * dir = p->args[1];
    
    if (p->argc < 2)
        dir = getenv("HOME");
    
    if ( chdir(dir) == 0) {
        print_info("Directory changed to \"%s\"\n", dir);
    }
    else {
        print_errno("cd");
    }
    
}

Job * search_process_by_number(int num) {
    Job * job = shell.jobs;
    int i = 1;
    
    while (job ) {
        
        // si el trabajo está en background y es distinto de un comando interno.
        if (!job->foreground && job->gpid != -1) { 
            
            if (i == num)
                return job;
            
            i++;
        }
        
        job = job->next;
    }
    
    return NULL;
}

Job * check_fg_bg_command_line(Process * p, const char * cmd) {
    int number;
    Job * job;
    
    if (p->argc < 2)  // bucamos el primero.
        number = 1;
    else
        number = atoi(p->args[1]);
    
    job = search_process_by_number(number);
    
    if (job == NULL && number == 1) {
        print_error("No hay trabajos pendientes.\n");
    }
    else if (job == NULL) {
        print_error("El número no coincide con ningún trabajo pendiente.\n");
    }
    
    return job;
}

void cmd_fg_handler(Process * p) {
    Job * fg_job;
    
    fg_job = check_fg_bg_command_line(p,internalCommands.str_cmd[cmd_fg]);

    if (fg_job)
        
        if (fg_job->type == NORMAL_JOB)
            put_job_foreground(fg_job);
        else 
            printf("Un trabajo round robin, no se puede traer a foreground.\n");
    
}

void cmd_bg_handler(Process * p) {
    Job * bg_job;
    
    bg_job = check_fg_bg_command_line(p,internalCommands.str_cmd[cmd_bg]);

    if (bg_job)
        
        if (bg_job->status != RUNNING)
            put_job_background(bg_job);
        else
            printf("El trabajo ya está en ejecución.\n");
}

void cmd_jobs_handler(Process * p) {
    Job * j = shell.jobs;
    int i = 1;
    
    while (j) {
        
        if (!j->foreground) {
            print_job_state(i,j);
            i++;
        }
        
        j = j->next;
    }
    
    if (i == 1)
        printf("No hay trabajos pendientes.\n");
    
}

void notify_and_clean_jobs() {
    Job * job = shell.jobs;
    int i = 1;
    
    printf(C_GREEN);
    while (job) {
        
        if (!job->foreground && IS_JOB_ENDED(job->status)) {
            print_job_state(i,job);
            block_sig(SIGCHLD);
            remove_job(&shell.jobs, job->gpid);
            unblock_sig(SIGCHLD);
        }
        else if (job->notify) {
            print_job_state(i, job);
            job->notify = 0; 
        }

        if (!job->foreground)
            i++;
        
        job = job->next;
    }
    printf(C_DEFAULT);fflush(stdout);
    
}

void cmd_exit_handler() {
    destroy_shell();
    printf("Bye\n");
    exit(0);
}

void config_internal_commands() {
    LINK_CMD(cmd_cd, cmd_cd_handler);
    LINK_CMD(cmd_fg, cmd_fg_handler);
    LINK_CMD(cmd_bg, cmd_bg_handler);
    LINK_CMD(cmd_jobs, cmd_jobs_handler);
    LINK_CMD(cmd_exit, cmd_exit_handler);
    LINK_CMD(cmd_rr, cmd_rr_handler);
}

// ---------------------------------------------------------------------------//
// ---------------------------------- MAIN------------------------------------//
// ---------------------------------------------------------------------------//

int main() {
    char * cmd;
    Job * job;

    init_shell(&shell);
    config_internal_commands();

    do {
        cmd = getCommand(&(shell.hist));    // 1. Leo el comando.
        notify_and_clean_jobs();            // 2. Notifico y elimino los trabajos pendientes.
        job = create_job(&shell.jobs,cmd);  // 3. Creo el trabajo nuevo.
        launch_job(job);                    // 4. Se ejecuta.
    } while (1);

    return 0;
}
