/**
 * Contiene la configuración global
 * 
 * @file  defs.h
 * @autor Víctor Manuel Ortiz Guardeño
 * @date  27/04/2017
 */

#ifndef _DEFS_H_
#define _DEFS_H_

// Commands parameters.
#define MAX_LINE_COMMAND 256
#define MAX_ARGS 32

// I/O Parameters.
#define TERM_PROMPT "SHELL > "
#define C_BLACK     "\x1b[0m"
#define C_RED       "\x1b[31;1;1m"
#define C_BLUE      "\x1b[34;1;1m"
#define C_BROWN     "\x1b[33;1;1m"
#define C_PURPLE    "\x1b[35;1;1m"
#define C_GREEN     "\x1b[32;1;1m"

#define C_INFO      C_BLUE
#define C_ERROR     C_RED
#define C_PROMPT    C_PURPLE
#define C_DEFAULT   C_BLACK

// Comandos de terminal.
#define CMDEXIT "exit"
#define CMDJOBS "jobs"
#define CMDFG   "fg"
#define CMDBG   "bg"
#define CMDCD   "cd"
#define CMDHIST "historial"

#endif
