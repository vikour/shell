/**
 * Contiene la información a cerca del historial. 
 * 
 * El historial, ofrece la funcionalidad de recordar qué comandos introdujo el 
 * usuario. Exiten dos tipos de entradas en el historial:
 * 
 * - Protegidas:
 *   Las entradas protegidas son aquellas que si se modifica el comando en el interior,
 *   pueden volver a su estado inicial llamando a la función ceanHistory().
 * 
 * - No protegidas:
 * 
 * @file  history.h
 * @autor Víctor Manuel Ortiz Guardeño
 * @date  27/04/2017
 */

#ifndef HISTORY_H
#define HISTORY_H

#include <defs.h>

typedef struct H_Node Node;
typedef Node * HistoryLine;

struct H_Node {
  char command[MAX_LINE_COMMAND];   // Comando de línea.
  char dirty;                       // 1 si la línea está sucia, 0 si no.
  char * backup;                    // copia de seguridad de la línea limpia.
  Node * prev;                      // Siguiente linea del historial.
  Node * next;                      // Anterior línea del historial.
};

struct S_History {
  Node * first;                     // Primera línea del historial.
  Node * last;                      // Última línea del historial.
  HistoryLine selected;             // Entrada seleccionada.
  int total;
};

typedef struct S_History History;

/**
 * Inicia el historial para su posterior utilización.
 * 
 * @param hist  Dirección del historial.
 */
void initHist(History * hist);

/**
 * Libera las referencias a memoria del historial.
 * 
 * @param hist Dirección del historial.
 */

void destroyHist(History * hist);

/**
 * Añade un comando al final del historial.
 * 
 * @param hist   Dirección del historial
 * @param cmd    Comando a añadir.
 */

void append(History * hist, char * cmd);

/**
 * Elimina la última entrada del historial.
 * 
 * @param hist   Dirección del historial.
 */

void removeLast(History * hist);

/**
 * Permite acceder a la primera entrada del historial.
 * 
 * @param hist   Dirección del historial.
 * @return       Primera entrada al historial, o NULL si no existiera.
 */

Node * getFirstCommand(History * hist);


/**
 * Permite acceder a la última entrada del historial.
 * 
 * @param hist   Dirección del historial.
 * @return       Última entrada al historial, o NULL si no existiera.
 */

Node * getLastCommand(History * hist);

/**
 * Esta función permite acceder a una linea del historial por su número. Devolverá
 * NULL si no existe tal linea.
 * 
 * @param hist  Historial
 * @param n     Número de línea.
 * @return      La línea del historial.
 */

HistoryLine getLine(History * hist, int n);

/**
 * Permite obtener la siguiente entrada a la pasada por argumento en el historial.
 * Esta función está pensada para navegar por el historial. Esta puede ser NULL
 * si no hay siguiente.
 * 
 * @param node  Entrada del historial.
 */

void nextCommand(HistoryLine * node);

/**
 * Permite obtener la entrada previa a la pasada por argumento en el historial.
 * Esta función está pensada para navegar por el historial. Esta puede ser NULL
 * si no hay anterior.
 * 
 * @param node  Entrada del historial.
 */

void prevCommand(HistoryLine * node);


/**
 * Permite conocer si la entrada al historial pasada como argumento está protegida
 * o no.
 * 
 * @param line
 * @return 
 */

char isEmptyEntry(HistoryLine line);

/**
 * Esta función permite conocer si la línea pasada como argumento, no está protegida.
 * 
 * @param line  Línea del historial.
 * @return 1 si no está protegida, 0 en el caso contrario.
 */

char isUnprotectEntry(HistoryLine line);

/**
 * Limpia todas las líneas sucias del historial.
 * 
 * @param hist  Dirección del historial.
 */

void cleanHistory(History * hist);

/**
 * Esta función convierte el nodo pasado como argumento en DIRTY.
 * 
 * @param node  Dirección del nodo.
 * 
 */

void dirtyNode(Node * node);

/**
 * Restaura el valor de un nodo DIRTY.
 * 
 * @param node Dirección de un nodo.
 */

void restoreNode(Node * node);

/**
 * Esta función añade al final del historial una linea vacía y desprotegida.

 * @param hist Dirección del historial.
 */

void appendUnprotectEntry(History * hist); // crea un nodo editable

/**
 * Esta función protege una línea DIRTY.
 * 
 * @param line Dirección de la línea DIRTY.
 */

void protectEntry(HistoryLine line);


#endif /* HISTORY_H */

