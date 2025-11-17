/*
 ============================================================================
 Name        : input.h
 Author      : Leonardo, Antonio, Francesco, Michele, Vincenzo
 Descrizione : Interfaccia per l'input system
               - Definizioni costanti per tasti speciali
               - Prototipo della funzione di lettura input
 ============================================================================
 */

#ifndef INPUT_H
#define INPUT_H

// Codici per i tasti speciali
#define CURSOR_UP     1
#define CURSOR_DOWN   2
#define CURSOR_LEFT   3
#define CURSOR_RIGHT  4
#define CURSOR_SELECT 5 // Invio
#define CURSOR_BACK   6 // ESC

#ifdef _WIN32
#include <conio.h>
#endif

// Funzione per leggere l'input
int getch_custom();

#endif
