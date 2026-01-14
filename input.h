/*
 ============================================================================
 Name        : input.h
 Descrizione : Interfaccia per il sistema di input multipiattaforma
               - Definizioni costanti per tasti speciali (frecce, INVIO, ESC)
               - Prototipo della funzione di lettura input personalizzata
               - Gestione automatica delle dipendenze per Windows/Linux
               - Codici standardizzati per navigazione menu e gameplay
               - Sistema unificato per WASD e frecce direzionali
               - Mappatura tasti speciali per tutte le piattaforme supportate
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
