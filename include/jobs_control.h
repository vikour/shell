/* 
 * Contiene todo lo relacionado a la gestión de trabajos en la shell. Incluye
 * las funciones para añadir trabajos, borras trabajos y las transisiones de
 * estado que se producen ante una señal de entrada.
 * 
 * @file  jobs_control.h
 * @autor Víctor Manuel Ortiz Guardeño
 * @date  30/04/2017
 */

#ifndef JOBS_CONTROL_H
#define JOBS_CONTROL_H

#include <defs.h>
#include <unistd.h>
#include <termios.h>

typedef enum {P_READY, P_RUNNING, P_STOPPED, P_SIGNALED, P_EXITED} ProcState;

struct T_Process {
    char * args[MAX_ARGS + 1];       // +1, por el NULL que indica el fin de la lista.
    int argc;                        // Número de argumentos.
    pid_t pid;                       // PID del proceso.
    ProcState state;
    int info;
    int num_job;
    struct T_Process * next;         // Siguiente proceso.
};

typedef struct T_Process Process;
typedef enum {JOB_READY,JOB_RUNNING,JOB_STOPPED,JOB_COMPLETED, JOB_SIGNALED} JobState;

#define IS_JOB_ENDED(s) ((s) == JOB_COMPLETED || (s) == JOB_SIGNALED)

struct T_Job {
    const char * command;             // Comando que inició el trabajo.
    struct termios tmodes;            // Modo de la terminal.
    char cargarModo;                  // Indica si se tiene que cargar el modo de la terminal al iniciar de nuevo.
    pid_t gpid;                       // pid del grupo de trabajo.
    char foreground;                  // 1. si está se ejecuta en background.
    int status;                       // estado del proceso.
    int * info;                       // Información acerca del estado.
    char notify;                      // Indica que se muestre el estado al usuario por cualquier motivo.
    Process * proc;                   // Lista de procesos del trabajo.
    struct T_Job * next;              // Siguiente trabajo.
};

typedef struct T_Job Job;
typedef Job * ListJobs;

/**
 * Inicializa la lista de trabajos para su posterior uso.
 * 
 * @param list_jobs   Dirección donde se creará la lista de trabajos.
 */

void init_list_jobs(ListJobs * list_jobs);

/**
 * Libera la memoria de la lista de trabajos alojada en la dirección pasada como
 * argumento.
 * 
 * @param list_jobs Dirección de la lista de trabajos.
 */

void destroy_list_jobs(ListJobs * list_jobs);

/**
 * Crea un trabajo a partir del comando dado como argumento. Si este comando
 * tiene tuberías, creará varios enlazados. Si el comando tiene un &, tomará como
 * que estará en background. Si hubiera algo después del &, se ignorará.
 * 
 * Este trabajo se añadirá al final de todos los trabajos de la lista pasada como
 * argumento.
 * 
 * NOTA: no se le asigna ningún PID, esto se supongo que se hará a posteriori, ya
 * que todos los trabajos son creados con el stado READY. Por lo tanto, para 
 * posteriores llamadas, será conveniente darle uno.
 * 
 * Por defecto, un trabajo se crea con las siguientes opciones:
 * 
 * - command    :    (Pasado como argumento)
 * - gpid       :    0
 * - termios    :    (Nada)
 * - cargarModo :    0
 * - foreground :    (Depende de &)
 * - status     :    job_ready
 * - info       :    0
 * - notify     :    0
 * - proc       :    (Lista de procesos según command)
 * 
 * @param list_jobs  Dirección de la lista de trabajos.
 * @param cmd        Comando que iniciará el trabajo.
 * @return           NULL si list_jobs o cmd es nulo o ""; un puntero hacia el 
 *                   trabajo creado, en otro caso.
 */

Job * create_job(ListJobs * list_jobs, const char * cmd);

/**
 * Elimina el trabajo con gpid pasado como argumento; si no existe, no borra nada.
 * 
 * @param list_jobs  Dirección de la lista de trabajos.
 * @param gpid       GPID del trabajo que se desea eliminar.
 */

void remove_job(ListJobs * list_jobs, pid_t gpid);

char is_job_n_running(Job * job, int i);
char is_job_n_stopped(Job * job, int i);
char is_job_n_completed(Job * job, int i, char * signaled);

#define is_job_running(j)    is_job_n_running((j), -1)
#define is_job_stopped(j)    is_job_n_stopped((j), -1)
#define is_job_completed(j,s)  is_job_n_completed((j), -1, (s))
#define is_job_foreground(j) is_job_running(j) && (j)->foreground
#define is_job_background(j) is_job_running(j) && !(j)->foreground

void mark_process(Job * job, int status, pid_t pid);

/**
 * Esta función analiza el estado pasado como argumento del grupo de procesos 
 * (obtenida con waitpid o alguna similar), para ver a qué estado siguiente 
 * tomará el trabajo pasado como argumento.
 * 
 * Las transiciones son:
 * - Ready -> Ejecución foreground  (exec = 1)  \ Se decide por el job->foreground
 * - Ready -> Ejecución background  (exec = 1)  /
 * - Ejecución foreground -> Detenido (por SIGSTOP o SIGTSTP)
 * - Ejecución foreground -> Completado (Por SIGEXITED)
 * - Ejecución foreground -> Signaled (Por cualquier señal de terminación)
 * - Ejecución foreground -> Ejecución bakground (FG)
 * - Ejecución background -> Detenido (por SIGSTOP)
 * - Ejecución background -> Completado (Por SIGEXITED)
 * - Ejecución background -> Signaled (Por cualquier señal de terminación)
 * - Detenido -> Ejecución foreground (Por SIGCONTINUE y foreground = 1)
 * - Detenido -> Ejecución background (Por SIGCONTINUE y foreground = 0)
 * - Detenido -> Signaled (Por cualquier señal de finalización)
 * 
 * En cualquier otro caso, no se modificará el estado del trabajo.
 * 
 * @param job         Dirección del trabajo.
 * @param status      Estado de la señal obtenida según waitpid o similar.
 * @param foreground  Si en el siguiente estado estará en foreground o no.
 * @param exec        Si se ha pasado ha ejecución, solo afecta en el caso de que
 *                    el trabajo esté en READY.
 */

void analyce_job_status(Job * job);

#endif /* JOBS_CONTROL_H */

