/*
 ============================================================================
 Name        : input.c
 Descrizione : Implementazione del sistema di input multipiattaforma
               - Lettura tasti senza echo (modalità raw del terminale)
               - Supporto completo per tasti speciali (frecce direzionali, ESC, INVIO)
               - Astrazione completa dalle differenze Windows/Linux/MacOS
               - Gestione non-bloccante delle sequenze ESC per Linux/MacOS con select()
               - Mapping unificato dei codici tasti per navigazione UI
               - Supporto sia per WASD che per frecce direzionali
               - Rilevamento sequenze multi-carattere per tasti funzione
               - Ripristino automatico impostazioni terminale dopo lettura
 ============================================================================
 */

#include "input.h"

#ifdef _WIN32
#include <conio.h>

int getch_custom() {
    int ch = _getch();
    if (ch == 0 || ch == 224) { // Tasti speciali Windows
        switch(_getch()) {
            case 72: return CURSOR_UP;
            case 80: return CURSOR_DOWN;
            case 75: return CURSOR_LEFT;
            case 77: return CURSOR_RIGHT;
            default: return ch;
        }
    }
    if (ch == 13) return CURSOR_SELECT;
    if (ch == 27) return CURSOR_BACK;
    return ch;
}

#else
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>     // SPOSTATO QUI
#include <sys/time.h>       // SPOSTATO QUI

int getch_custom() {
    struct termios oldt, newt;
    int ch;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    ch = getchar();
    
    // Gestione ESC senza bloccare
    if (ch == 27) {
        // Verifica se ci sono caratteri disponibili immediatamente (non bloccante)
        struct timeval tv = {0, 0};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        
        // Se c'è un carattere disponibile, è probabilmente parte di una sequenza
        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
            int ch2 = getchar();
            if (ch2 == 91) { // Carattere '['
                // Verifica se c'è il terzo carattere
                FD_ZERO(&fds);
                FD_SET(STDIN_FILENO, &fds);
                if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
                    int ch3 = getchar();
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    switch(ch3) {
                        case 65: return CURSOR_UP;
                        case 66: return CURSOR_DOWN;
                        case 67: return CURSOR_RIGHT;
                        case 68: return CURSOR_LEFT;
                        default: return CURSOR_BACK;
                    }
                }
            }
        }
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return CURSOR_BACK;
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    
    if (ch == 10) return CURSOR_SELECT;
    return ch;
}
#endif