/**
 * Este fichero contiene las funciones de entrada y salida de la shell.
 * 
 * @file  IOModule.h
 * @autor Víctor Manuel Ortiz Guardeño
 * @date  08/05/2017
 */


#ifndef _IOModule_H_
#define _IOModule_H_

#include <history.h>

#define print_info(s,...)  printf(C_INFO s C_DEFAULT, ## __VA_ARGS__); fflush(stdout)
#define print_error(s,...) printf(C_ERROR s C_DEFAULT, ## __VA_ARGS__); fflush(stdout)
#define print_errno(s, ...) perror(C_ERROR s); printf(C_DEFAULT); fflush(stdout)

char * getCommand(History * hist);

#endif
