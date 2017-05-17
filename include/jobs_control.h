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

typedef enum {READY,RUNNING,STOPPED,SIGNALED,COMPLETED} State;

struct T_Process {
    char * args[MAX_ARGS + 1];       // +1, por el NULL que indica el fin de la lista.
    int argc;                        // Número de argumentos.
    pid_t pid;                       // PID del proceso.
    State state;
    int info;
    int num_job;
    struct T_Process * next;         // Siguiente proceso.
};

typedef struct T_Process Process;

typedef enum {NORMAL_JOB,RR_JOB} TypeJob;
#define IS_JOB_ENDED(s) ((s) == COMPLETED || (s) == SIGNALED)

struct T_Job {
    const char * command;             // Comando que inició el trabajo.
    struct termios tmodes;            // Modo de la terminal.
    char cargarModo;                  // Indica si se tiene que cargar el modo de la terminal al iniciar de nuevo.
    pid_t gpid;                       // pid del grupo de trabajo.
    char foreground;                  // 1. si está se ejecuta en background.
    State status;                       // estado del proceso.
    int * info;                       // Información acerca del estado.
    char notify;                      // Indica que se muestre el estado al usuario por cualquier motivo.
    int active;
    int total;
    TypeJob type;
    char respawnable;
    Process * proc;                   // Lista de procesos del trabajo.
    int time_out;                     // Indica si tiene time out asignado.
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
void dup_job_command(Job * job);

/**
 * Elimina el trabajo con gpid pasado como argumento; si no existe, no borra nada.
 * 
 * @param list_jobs  Dirección de la lista de trabajos.
 * @param gpid       GPID del trabajo que se desea eliminar.
 */

void remove_job_n(ListJobs * list_jobs, pid_t gpid,int n);
void reenumerate_job(Job * job);

#define remove_job(l,g)   remove_job_n((l),(g),-1)

char is_job_n_running(Job * job, int i);
char is_job_n_stopped(Job * job, int i);
char is_job_n_completed(Job * job, int i, char * signaled);

#define is_job_running(j)      is_job_n_running((j), -1)
#define is_job_stopped(j)      is_job_n_stopped((j), -1)
#define is_job_completed(j,s)  is_job_n_completed((j), -1, (s))
#define is_job_foreground(j)   (is_job_running(j) && (j)->foreground)
#define is_job_background(j)   (is_job_running(j) && !(j)->foreground)

void mark_process(Job * job, int status, pid_t pid);
Job * search_job_by_process(ListJobs jobs,pid_t pid);
void analyce_job_status(Job * job);
void kill_job(Job * job, int n, int sig);

#endif /* JOBS_CONTROL_H */

