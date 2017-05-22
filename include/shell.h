/**
 * Contiene las declaraciones de la imagen de shell, con todas sus variables usadas.
 * También contiene la información a cerca de qué comandos internos hay.
 * 
 * @file  shell.h
 * @autor Víctor Manuel Ortiz Guardeño
 * @date  08/05/2017
 */

#ifndef SHELL_H
#define SHELL_H

#include <unistd.h>
#include <termios.h>
#include <history.h>
#include <IOModule.h>
#include <defs.h>
#include <jobs_control.h>

struct T_Shell {
  int fdin;
  pid_t pid;
  History hist;
  ListJobs jobs;
  char sigalarm_on;
  struct termios mode;
} shell;

typedef struct T_Shell Shell;

struct __InfoProcess {
    pid_t pid;
    pid_t ppid;
    long threads;
    int childs;
    char comm[100];
    struct __InfoProcess * next;
};

typedef struct __InfoProcess InfoProcess;
typedef InfoProcess * ListChildren;

// trabajos internos de la terminal.
typedef struct {
    int count;
    const char ** str_cmd;
    int * fork;
    void (*handler[])(Process *);
} InternalCommandInfo;

// Configuración del nómbre del comando, y su cadena asociada.
// CMD(enum_name, str_name, 1 if forked)
#define INTERNAL_COMMAND  \
   CMD(cmd_exit,    CMDEXIT,   0) \
   CMD(cmd_fg,      CMDFG,     0) \
   CMD(cmd_bg,      CMDBG,     0) \
   CMD(cmd_jobs,    CMDJOBS,   1) \
   CMD(cmd_cd,      CMDCD,     0) \
   CMD(cmd_rr,      CMDRR,     0) \
   CMD(cmd_hist,    CMDHIST,   1) \
   CMD(cmd_timeout, CMDTOUT,   0) \
   CMD(cmd_children, CMDCHILD, 1)

// Creación de la enumeración
enum internal_command_names {
    #define CMD(c,s,f) c,
    INTERNAL_COMMAND
    #undef CMD
};

// Generación de la estructura de información.
#define CAT_NOEXPAND(A) A
#define CAT(A) CAT_NOEXPAND(A)

InternalCommandInfo internalCommands = {
    .count = (
        #define CMD(c,s,f) 1 +
        INTERNAL_COMMAND
        #undef CMD
    0),
    .handler = {
        #define CMD(c,s,f) NULL,
        INTERNAL_COMMAND
        #undef CMD
    },
    .str_cmd = (const char *[]) {
        #define CMD(c,s,f) CAT(s),
        INTERNAL_COMMAND
        #undef CMD
    },
    .fork = (int []) {
        #define CMD(c,s,f) (f),
        INTERNAL_COMMAND
        #undef CMD
    }
};

#define LINK_CMD(c,f)   internalCommands.handler[(c)] = (f)
#define ICMD_FORK(c)    internalCommands.fork[(c)]
#define ICMD_HANDLER(c) internalCommands.handler[(c)]
#define ICMD_STR(c)     internalCommands.str_cmd[(c)]
#define ICMD_TOTAL      internalCommands.count

#undef CAT_NOEXPAND
#undef CAT

#endif /* SHELL_H */

