/*
 ============================================================================
 Name        : input.c
 Author      : Leonardo, Antonio, Francesco, Michele, Vincenzo
 Descrizione : Gestione input multi-piattaforma
               - Lettura tasti senza echo
               - Supporto per tasti speciali (frecce, ESC)
               - Astrazione dalle differenze Windows/Linux
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
#include <termios.h>
#include <unistd.h>

int getch_custom() {
    struct termios oldt, newt;
    int ch;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    // Gestione tasti freccia (sequenze ANSI)
    if (ch == 27) {
        if (getchar() == 91) {
            switch(getchar()) {
                case 65: return CURSOR_UP;
                case 66: return CURSOR_DOWN;
                case 67: return CURSOR_RIGHT;
                case 68: return CURSOR_LEFT;
            }
        }
        return CURSOR_BACK;
    }
    if (ch == 10) return CURSOR_SELECT;
    return ch;
}
#endif
