/**
 * Implementación del historial
 * 
 * @file  history.c
 * @autor Víctor Manuel Ortiz Guardeño
 * @date  27/04/2017
 */

#include <history.h>
#include <stdlib.h>
#include <string.h>

void initHist(History * hist) {
    hist->first = NULL;
    hist->last = NULL;
}

void destroyHist(History * hist) {
    Node * curr = hist->first;
    Node * prev = NULL;
    
    hist->selected = NULL;
    
    while (curr) {
        prev = curr;
        curr = curr->next;

        if (prev->backup)
            free(prev->backup);
        
        free(prev);
    }
    
}

static Node * createNode(char * cmd, Node * next, Node * prev) {
    Node * tmp;
    
    tmp = (Node *) malloc(sizeof(Node));
    
    if (cmd)
        strcpy(tmp->command, cmd);
    
    tmp->next = next;
    tmp->prev = prev;
    tmp->dirty = 0;
    tmp->backup = NULL;
}


void append(History * hist, char * cmd) {
    Node * node;
    
    node = createNode(cmd, NULL, hist->last);
    
    if (hist->last)
        hist->last->next = node;
    
    hist->last = node;
    
    if (!hist->first)
        hist->first = node;
    
}

void appendUnprotectEntry(History * hist) {
    append(hist, NULL);
    hist->last->dirty = 1;
}

Node * getLastCommand(History * hist) {
    return hist->last;
}

Node * getFirstCommand(History * hist) {
    return hist->first;
}

void nextCommand(HistoryLine* node) {
    
    if (*node)
        *node = (*node)->next;
    
}

void prevCommand(HistoryLine * node) {
    
    if (*node)
        *node = (*node)->prev;
    
}

void cleanHistory(History * hist) {
    Node * curr = hist->first;
    
    while (curr) {
        restoreNode(curr);
        curr = curr->next;
    }
    
}

void dirtyNode(Node * node) {
    
    if (node && !(node->dirty) && !node->backup)  {
        node->backup = (char * ) malloc(sizeof(char) * MAX_LINE_COMMAND);
        strcpy(node->backup, node->command);
        node->dirty = 1;
    }
    
}

void restoreNode(Node * node) {
    
    if (node && node->dirty) {
        strcpy(node->command, node->backup);
        free(node->backup);
        node->backup = NULL;
        node->dirty = 0;
    }
    
}

void removeLast(History * hist) {
    Node * rm = hist->last;
    
    if (rm) {
        
        if (rm->prev) {
            rm->prev->next = NULL;
            hist->last = rm->prev;
        }
        else {
            hist->first = NULL;
            hist->last = NULL;
        }
        
        if (rm->backup)
            free(rm->backup);
        
        free(rm);
    }
}



void protectEntry(HistoryLine line) {
    line->dirty = 0;
    
}

char isUnprotectEntry(HistoryLine line) {
    return line->dirty && !line->backup;
}


char isEmptyEntry(HistoryLine line) {
    return strlen(line->command) == 0;
}
