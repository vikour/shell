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
    
    printf("\t%s\n", job->command);
}

/**
 * El manejador de SIGCHLD actualiza la lista de trabajos.
 */

void handler_sigchld(int sig) {
    Job * j = shell.jobs;
    int status;
    pid_t pid;
    
    while (j) {
        
        pid = waitpid(- j->gpid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        
        // pid > 0 significa "alguien notificó su estado", si devuelve 0, es que no está
        // el hijo disponible, y -1 si no existe el hijo.
        if (pid > 0) {
            mark_process(j,status,pid);
            analyce_job_status(j);
            // Se notifica al usuario, si el proceso se detuvo en background
            j->notify = (j->status == STOPPED || IS_JOB_ENDED(j->status)) && !j->foreground ;
        }
        
        
        j = j->next;
        
    }
    
}

void block_sigchld() {
    sigset_t block_sigchld;
    
    sigemptyset(&block_sigchld);
    sigaddset(&block_sigchld, SIGCHLD);
    sigprocmask(SIG_BLOCK, &block_sigchld, NULL);
}

void unblock_sigchld() {
    sigset_t block_sigchld;
    
    sigemptyset(&block_sigchld);
    sigaddset(&block_sigchld, SIGCHLD);
    sigprocmask(SIG_UNBLOCK, &block_sigchld, NULL);
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
    
    // Manejamos la señal SIGCHLD
    signal(SIGCHLD, handler_sigchld);
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
    block_sigchld();
    
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
    unblock_sigchld();
    
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
    
    if (icmd < 0) { // Si no es un comando interno, se ejecuta el ejecutable.
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
    int fdp[2];
    int outfile, infile;
    
    outfile = STDOUT_FILENO;
    infile  = shell.fdin;
    
    while (p) {
        p->pid = fork(); 
        
        // Configuración de pipes.
        if ( pipe(fdp) < 0) {
            print_error("Error al crear la tubería.");
            exit(1);
        }
        else {
            outfile = fdp[1];
        }
        
        if (p->pid == 0) 
            launch_process(p, infile, outfile,job->gpid,job->foreground);
        else {
            
            if (job->gpid == 0)
                job->gpid = p->pid;
            
            setpgid(p->pid, job->gpid);
            mark_process(job,0,p->pid);
        }
        
        // configuracion de la entrada.
        infile = fdp[0];
        
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
        block_sigchld();
        remove_job(&shell.jobs, -1);
        unblock_sigchld();
    }
    else
        launch_forked_job(job);
    
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
        put_job_foreground(fg_job);
}

void cmd_bg_handler(Process * p) {
    Job * bg_job;
    
    bg_job = check_fg_bg_command_line(p,internalCommands.str_cmd[cmd_bg]);

    if (bg_job)
        put_job_background(bg_job);
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
            block_sigchld();
            remove_job(&shell.jobs, job->gpid);
            unblock_sigchld();
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
}

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
