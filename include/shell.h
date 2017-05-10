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
  struct termios mode;
} shell;

typedef struct T_Shell Shell;

// trabajos internos de la terminal.
typedef struct {
    int count;
    const char ** str_cmd;
    void (*handler[])(Job *);
} InternalCommandInfo;

// Configuración del nómbre del comando, y su cadena asociada.
#define INTERNAL_COMMAND  \
   CMD(cmd_exit, CMDEXIT) \
   CMD(cmd_fg,   CMDFG)   \
   CMD(cmd_bg,   CMDBG)   \
   CMD(cmd_jobs, CMDJOBS) \
   CMD(cmd_cd,   CMDCD)

// Creación de la enumeración
enum internal_command_names {
    #define CMD(c,s) c,
    INTERNAL_COMMAND
    #undef CMD
};

// Generación de la estructura de información.
#define CAT_NOEXPAND(A) A
#define CAT(A) CAT_NOEXPAND(A)

InternalCommandInfo internalCommands = {
    .count = (
        #define CMD(c,s) 1 +
        INTERNAL_COMMAND
        #undef CMD
    0),
    .handler = {
        #define CMD(c,s) NULL,
        INTERNAL_COMMAND
        #undef CMD
    },
    .str_cmd = (const char *[]) {
        #define CMD(c,s) CAT(s),
        INTERNAL_COMMAND
        #undef CMD
    }
};

#define LINK_CMD(c,f) internalCommands.handler[(c)] = (f)

#undef CAT_NOEXPAND
#undef CAT

#endif /* SHELL_H */

