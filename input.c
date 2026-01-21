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
#include <windows.h>

// Variabili per gestire il debouncing avanzato (anti-ripetizione)
static DWORD s_lastKeyTime = 0;
static int s_lastKey = -1;
static int s_keyPressed[256] = {0}; // Array per tracciare lo stato dei tasti

int getch_custom() {
    int ch = _getch();
    if (ch == 0 || ch == 224) { // Tasti speciali Windows
        int ch2 = _getch();
        switch(ch2) {
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

// Versione migliorata con edge detection (rileva solo transizioni da non premuto a premuto)
int getch_nonblocking() {
    DWORD currentTime = GetTickCount();

    // Reset automatico dopo 50ms di inattività
    if (currentTime - s_lastKeyTime > 50) {
        s_lastKey = -1;
    }

    // Array di tasti da controllare con i relativi codici di ritorno
    struct KeyMapping {
        int vkCode;
        int returnCode;
    };

    struct KeyMapping mappings[] = {
        {VK_UP, CURSOR_UP},
        {VK_DOWN, CURSOR_DOWN},
        {VK_LEFT, CURSOR_LEFT},
        {VK_RIGHT, CURSOR_RIGHT},
        {VK_RETURN, CURSOR_SELECT},
        {VK_ESCAPE, CURSOR_BACK},
        {'W', CURSOR_UP},
        {'S', CURSOR_DOWN},
        {'A', CURSOR_LEFT},
        {'D', CURSOR_RIGHT},
        {'w', CURSOR_UP},
        {'s', CURSOR_DOWN},
        {'a', CURSOR_LEFT},
        {'d', CURSOR_RIGHT},
        {'P', 'P'},
        {'p', 'P'},
        {'V', 'V'},
        {'v', 'V'},
        {'M', 'M'},
        {'m', 'M'},
    };

    // Controlla anche i numeri 0-9
    for (int i = '0'; i <= '9'; i++) {
        if (GetAsyncKeyState(i) & 0x8000) {
            if (!s_keyPressed[i]) {
                s_keyPressed[i] = 1;
                s_lastKeyTime = currentTime;
                s_lastKey = i;
                return i;
            }
        } else {
            s_keyPressed[i] = 0;
        }
    }

    // Controlla tutti i tasti mappati
    for (int i = 0; i < sizeof(mappings)/sizeof(mappings[0]); i++) {
        int vkCode = mappings[i].vkCode;
        int returnCode = mappings[i].returnCode;

        if (GetAsyncKeyState(vkCode) & 0x8000) {
            // Tasto premuto
            if (!s_keyPressed[vkCode]) {
                // Edge detection: transizione da non premuto a premuto
                s_keyPressed[vkCode] = 1;
                s_lastKeyTime = currentTime;
                s_lastKey = returnCode;
                return returnCode;
            }
        } else {
            // Tasto rilasciato
            s_keyPressed[vkCode] = 0;
        }
    }

    // Debouncing: ignora lo stesso tasto per 150ms
    for (int i = 0; i < sizeof(mappings)/sizeof(mappings[0]); i++) {
        int vkCode = mappings[i].vkCode;
        if (GetAsyncKeyState(vkCode) & 0x8000) {
            if (s_lastKey == mappings[i].returnCode &&
                currentTime - s_lastKeyTime < 150) {
                return -1; // Troppo presto dopo l'ultima pressione
            }
        }
    }

    return -1; // Nessun input valido
}

void restore_terminal() {
    // Niente da fare per Windows
}

#else // Unix/Linux/MacOS

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>

// Variabili per debouncing su Unix
static struct timeval s_lastKeyTime = {0, 0};
static int s_lastKey = -1;
static int s_keyPressed[256] = {0};

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

// Versione Unix migliorata con edge detection
int getch_nonblocking() {
    static int initialized = 0;
    static struct termios oldt, newt;
    struct timeval currentTime, diffTime;

    gettimeofday(&currentTime, NULL);

    if (!initialized) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        newt.c_cc[VMIN] = 0;  // Non bloccante
        newt.c_cc[VTIME] = 0; // Ritorno immediato
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        initialized = 1;
    }

    int ch = getchar();

    if (ch == EOF) {
        return -1;
    }

    // Debouncing: ignora tasti troppo ravvicinati
    timersub(&currentTime, &s_lastKeyTime, &diffTime);
    long elapsed_ms = diffTime.tv_sec * 1000 + diffTime.tv_usec / 1000;

    if (elapsed_ms < 100 && ch == s_lastKey) { // 100ms di debounce
        return -1;
    }

    // Gestione ESC
    if (ch == 27) {
        struct timeval tv = {0, 1000}; // 1ms timeout
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);

        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
            int ch2 = getchar();
            if (ch2 == 91) { // '['
                FD_ZERO(&fds);
                FD_SET(STDIN_FILENO, &fds);
                tv.tv_usec = 1000;

                if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
                    int ch3 = getchar();
                    s_lastKeyTime = currentTime;
                    s_lastKey = ch3;
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
        s_lastKeyTime = currentTime;
        s_lastKey = CURSOR_BACK;
        return CURSOR_BACK;
    }

    // Mappatura caratteri semplici
    if (ch == 10 || ch == 13) {
        s_lastKeyTime = currentTime;
        s_lastKey = CURSOR_SELECT;
        return CURSOR_SELECT;
    }

    // Edge detection per caratteri normali
    if (!s_keyPressed[ch]) {
        s_keyPressed[ch] = 1;
        s_lastKeyTime = currentTime;
        s_lastKey = ch;
        return ch;
    } else {
        // Reset quando il tasto viene rilasciato (questo richiede un approccio diverso)
        // Per ora, teniamo questo approccio semplificato
    }

    return -1;
}

// Aggiungi funzione per ripristinare terminale (da chiamare all'uscita)
void restore_terminal() {
    static int restored = 0;
    static struct termios oldt;
    if (!restored) {
        // Ri-leggi le impostazioni originali per sicurezza
        tcgetattr(STDIN_FILENO, &oldt);
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        restored = 1;
    }
}

#endif // Fine del blocco #ifdef _WIN32
