/**
 * Implementación del módulo de entrada y salida.
 * 
 * @file  inputModule.c
 * @autor Víctor Manuel Ortiz Guardeño
 * @date  27/04/2017
 */

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

// Includes form shell
#include <defs.h>
#include <history.h>
#include <IOModule.h>

#define KEY_ENTER      10
#define KEY_SUP        127

#define EMACS_FORWARD  6
#define EMACS_BACKWARD 2
#define EMACS_PREVIOUS 16
#define EMACS_NEXT     14
#define EMACS_START_L  1
#define EMACS_END_LIN  5
#define EMACS_DELETE   4

#define CUR_SAVE "\033[s"
#define CUR_REST "\033[u"

static char getch() {
    int shell_terminal = STDIN_FILENO;
    struct termios conf;
    struct termios conf_new;
    char c;
    
    // leemos la configuración actual.
    tcgetattr(shell_terminal, &conf);
    conf_new = conf;
    
    conf_new.c_lflag &= (~(ICANON | ECHO));
    conf_new.c_cc[VTIME] = 0;
    conf_new.c_cc[VMIN]  = 1;
    
    tcsetattr(shell_terminal, TCSANOW, &conf_new);
    
    c = getc(stdin);
    
    tcsetattr(shell_terminal, TCSANOW, &conf);
    
    return c;
}

/**
 * Llena el array con el caracter pasado como argumento.
 * 
 * @param array   Array destino.
 * @param length  Longitud del array.
 * @param c       Caracter que se introducira en el array.
 */

static void fill(char * buff, int length, char c) {
    int i = 0;
    
    for (; i < length ; i++)
        buff[i] = c;
    
}

/**
 * Imprime N espacios en la salida estandar.
 * 
 * @param n  Número de espacios a escribir.
 */

static void printNSpaces(int n) {
    int i;
    
    for (i = 0 ; i < n ; i++)
        putchar(' ');
    
}

/**
 * Desplaza los caracteres del array desde la posición hasta una posición determinada.
 * El límite es MAX_COMMAND_LINE.
 * 
 * @param buff     array de carácteres.
 * @param pos      posición desde donde se empieeza a mover carácteres.
 * @param length   posición final.
 */

static void shiftRight(char * buff, int pos, int length) {
    // uso la longitud como índice hacia atrás, la idea es dejar un hueco en la posición "pos"
    
    while ( length < MAX_LINE_COMMAND && length != pos) {
        buff[length] = buff[length - 1];
        length--;
    }

}

/**
 * Desplaza los caracteres del array hasta que encuentre el fin de la cadena o llegue a la longitud.
 * 
 * @param buff    Array de carácteres.
 * @param pos     Posición desde donde empezar.
 * @param length  Longitud de la caracteres.
 */

static void shiftLeft(char * buff, int pos, int length) {
    
    while (pos <= length && buff[pos - 1] != '\0') {
        buff[pos - 1] = buff[pos];
        pos++;
    }
    
}

/**
 * Limpia el prompt de la terminal introduciendo espacios.
 * 
 * pre: se ha establecido antes una posición del prompt
 * 
 * @param length  Número de espacios a introducir.
 */

static void clearPrompt(int length) {
    printf(CUR_REST);
    printNSpaces(length);  
}

/**
 * Procesa la pulsación de un caracter desde teclado. Actúa convirtiendo en dirty la entrada
 * del historial, si esta estuviera limpia(clean).
 *
 * @param line    Linea del historial que se va a modificar.
 * @param cursor  Referencia al cursor.
 * @param c       Caracter que se va a procesar.
 * @param length  Referencia a la longitud de la palabra dentro del buffer.
 */

static void characterProcess(HistoryLine line, int * cursor, int * length, char c) {

    if (isprint(c) && (*cursor) < MAX_LINE_COMMAND - 1) {
        
        if (!line->dirty)
            dirtyNode(line);
        
        shiftRight(line->command, *cursor, *length);    // Desplazo hacia la derecha.
        line->command[*cursor] = c;                     // Guardo el caracter en el hueco.
        (*length)++;                           // Incremento la longitud.
        (*cursor)++;                           // Muevo el cursor a la derecha.
        
    }
    
}

static void upArrowProcess(HistoryLine * line, int * cursor, int * length) {
    HistoryLine back = *line;
    
    prevCommand(&back);
    
    if (back) {
        *line = back;                             // Cambio la línea por la anterior.
        *cursor = strlen((*line)->command);       // ajusto el cursor al final de la linea.
        clearPrompt(*length);                     // borro la palabra que estaba en prompt
        *length = *cursor;                        // Ajusto la nueva longitud dela palabra.
    }
    
}

static void downArrowProcess(HistoryLine * line, int * cursor, int * length) {
    HistoryLine next = *line;
    
    nextCommand(&next);
    
    if (next) {
        *line = next;                             // Cambio la línea por la siguiente.
        *cursor = strlen((*line)->command);       // ajusto el cursor al final de la linea.
        clearPrompt(*length);                     // borro la palabra que estaba en prompt
        *length = *cursor;                        // Ajusto la nueva longitud dela palabra.
    }
    
}

static void leftArrowProcess(int * cursor) {
    
    if (*cursor > 0)
        (*cursor)--;
    
}

static void rigthArrowProcess(int * cursor, int lenght) {
    
    if (*cursor < lenght)
        (*cursor)++;
    
}

/**
 * Borra un caracter hacia la izquierda desde la posición del cursor, en la línea
 * actual.
 * 
 * @param line     Línea seleccionada del historial
 * @param cursor   Posición del cursor.
 * @param length   Longitud de la palabra escrita en la linea.
 */

static void borrar(HistoryLine line, int * cursor, int * length) {
    
    if (*cursor > 0) {
        
        if (!line->dirty)
            dirtyNode(line);
        
        shiftLeft(line->command, *cursor, *length);
        (*cursor)--;
        (*length)--;
    }
    
}

/**
 * Borra un caracter hacia la derecha desde la posición del cursor, en la línea
 * actual.
 * 
 * @param line     Línea seleccionada del historial
 * @param cursor   Posición del cursor.
 * @param length   Longitud de la palabra escrita en la linea.
 */

static void suprimir(HistoryLine line, int * cursor, int * length) {
    
    if (*cursor != *length) {
        
        if (!line->dirty)
            dirtyNode(line);
        
        shiftLeft(line->command, *cursor + 1, *length);
        (*length)--;
    }
    
}

/**
 * Imprime el comando por la pantalla, situando el cursor en la posición correcta.
 * 
 * @param buff        Buffer con el comando.
 * @param cursor      Posición del cursor.
 * @param length      Longitud del comando.
 */

static void printCommand(const char * buff, int cursor, int length) {

    clearPrompt(length+1);   // Borra el comando de la pantalla (+1 por si se borró el último)
    printf(CUR_REST"%s"CUR_REST, buff);     // Restauro y imprimo el comando actual.
    
    if (cursor != 0)
        printf("\033[%dC", cursor);     // Muevo el cursor a la posición correcta.
}

void parse_background_characters(char * cmd) {
    int cur, length;
    char * reject = "&+", ch = 0;
    
    length = strlen(cmd);
    cur = 0;
    
    if ( strcspn(cmd,reject) != length) {
        
        while (cur < length) {
            
            if (cmd[cur] == '&' || cmd[cur] == '+') {
                
                if (ch == 0)
                    ch = cmd[cur];
                else if (ch == '&' && cmd[cur] == '+')
                    ch = '+';
                
                shiftLeft(cmd,cur + 1, length);
                length--;                
                
                if (cmd[cur] == ' ') {
                    shiftLeft(cmd,cur + 1, length);
                    length--;
                }
                
                
            }
            
            cur++;
        }
        
        if ( cmd[length - 1] != ' ') {
            cmd[length] = ' ';
            length++;
        }
        
        cmd[length] = ch;
        cmd[length+1] = '\0';
    }
    
}

void parse_history_commands(History * hist, char * cmd) {
    char * rptr = strstr(cmd,CMDHIST);                        // Puntero de lectura.
    char aux[MAX_LINE_COMMAND];
    char * wptr = aux;                                        // Puntero de escritura.
    char * mark,                                              // Marca de después de una escritura.
         * mark2;                                             // Marca de inicio de lectura.
    int num, length;
    HistoryLine line;
    
    if (!rptr)
        return;
    
    mark = rptr;                                              // Por defecto
    
    do {
        mark2 = rptr;                                         // marcamos el inicio de la lectura.
        rptr += strlen(CMDHIST) + 1;                          // Avanzamos toda la palabra del 'historial' y un espacio.
        
        while (*rptr == ' ')                                  // Avanzamos todos los espacios que halla de sobra.
            rptr++;
        
        if (isdigit(*rptr) && (num = atoi(rptr)) <= hist->total) { // Si tal carácter es un dígito, y el número está en el rango del historial...
            strncpy(wptr,mark,mark2-mark);                        // Copiamos la diferencia entre las dos marcas.
            wptr += mark2-mark;                                   // avanzamos con el puntero.
            // copiamos el comando que es.
            line = getLine(hist, num);
            length = strlen(line->command);
            strncpy(wptr,line->command,length);
            wptr += length;
            // fin de copiado de comando.
            while (rptr && *rptr != ' ' && *rptr != '\0')         // Quitamos espacios y posible final.
                rptr++;
        }
        else {                                                // Si no, copiamos todo y avanzamos con el puntero.
            strncpy(wptr,mark,rptr-mark+1);
            wptr += rptr-mark;
        }                                                     // Fin si.
        
        mark = rptr;                                         // marca de postescritura.
        rptr = strstr(rptr, CMDHIST);                        // buscamos la siguiente palabra historial.
        
    } while (rptr && *rptr != '\0');                         // MIENTRAS, halla algo que leer.
    
    while (*mark != '\0') {                                  // Copiamos los restos.
        *wptr = *mark;
        wptr++;
        mark++;
    }
    *wptr = '\0';                                            // Marcamos el fin de línea.
    parse_background_characters(aux);
    strcpy(cmd, aux);                                        // Copiamos el resultado al comando pasado como argumento.
}

char * getCommand(History * hist) {
    int cursor = 0;                         // Posición del cursor.
    int length = 0;                         // Longitud del comando escrito por el usuario.
    char sec[3];                            // Acumulación de 3 carácteres (necesaria para las fechas)
    char exit = 0;
    HistoryLine lineSelected;
    
    printf(C_PROMPT TERM_PROMPT C_DEFAULT""CUR_SAVE"");
    
    // Pre history.
    appendUnprotectEntry(hist);                  // Introduzco un nodo no protegido. (directamente editable)
    lineSelected = getLastCommand(hist);         // Selecciono la última línea.
    // end pre history
    
    fill(lineSelected->command,MAX_LINE_COMMAND,'\0');
    
    do {
        sec[0] = getch();
        
        switch(sec[0]) {
            
            case 27:
                sec[1] = getch();
                
                if (sec[1] == 91) { // 27, 91 ...
                    sec[2] = getch();
                    
                    switch(sec[2]) {
                        
                        case 51: // Suprimir
                            suprimir(lineSelected, &cursor,&length);
                            getch(); // Se queda un caracter basura (~)
                            break;
                            
                        case 65: // Abajo
                            upArrowProcess(&lineSelected, &cursor, &length);
                            break;
                        
                        case 66: // Abajo
                            downArrowProcess(&lineSelected, &cursor, &length);
                            break;
                        
                        case 67: // Derecha
                            rigthArrowProcess(&cursor, length);
                            break;
                        
                        case 68: // Izquierda
                            leftArrowProcess(&cursor);
                            break;
                        
                        case KEY_ENTER:
                            exit = 1;
                            break;
                            
                        default:
                            characterProcess(lineSelected, &cursor, &length, sec[2]);
                            
                    }
                }
                else if (sec[1] != KEY_ENTER)
                    characterProcess(lineSelected, &cursor, &length, sec[1]);
                else 
                    exit = 1;
                    
                break;
                
            case EMACS_BACKWARD:
                leftArrowProcess(&cursor);
                break;
                
            case EMACS_FORWARD:
                rigthArrowProcess(&cursor, length);
                break;
                
            case EMACS_PREVIOUS:
                upArrowProcess(&lineSelected,&cursor,&length);
                break;
                
            case EMACS_NEXT:
                downArrowProcess(&lineSelected, &cursor, &length);
                break;
                
            case EMACS_START_L:
                cursor = 0;
                break;
                
            case EMACS_END_LIN:
                cursor = strlen(lineSelected->command);
                break;
                
            case EMACS_DELETE:
                strcpy(lineSelected->command, CMDEXIT);
                length = 4;
                exit = 1;
                break;
                
            case KEY_ENTER:
                exit = 1;
                break;
                
            case KEY_SUP:
                borrar(lineSelected, &cursor, &length);
                break;
                
            default:
                characterProcess(lineSelected, &cursor, &length, sec[0]);
        }
        
        // Impresión del carácter.
        if (!exit)
            printCommand(lineSelected->command, cursor, length);
        else
            putchar('\n');

    } while (!exit);
    
    // Copiado al historial.
    if (isEmptyEntry(lineSelected))  // Si es una linea vacía, se borra.
        removeLast(hist);
    else {
        
        if (!isUnprotectEntry(lineSelected)) {                            // Si no es la última linea...
            strcpy(getLastCommand(hist)->command, lineSelected->command); // Volcamos el comando a la última linea            
            lineSelected = getLastCommand(hist);                          // Seleccionamos la última linea.            
        }
        
        protectEntry(lineSelected);  // Protegemos la última línea.
        
    }
    
    cleanHistory(hist);
    
    if (length == 0) // No se introdujo nada (linea vacía);
        return NULL;
    else {
        parse_history_commands(hist,lineSelected->command);
        
        return lineSelected->command;
    }
}

