/*
 ============================================================================
 Name        : main.c
 Descrizione : Macchina a stati principale e loop di controllo del gioco Sudoku X
               - Gestione completa di 7 stati di gioco (MENU, DIFFICULTY, LOADMENU, PLAYING, PAUSED, WIN, GAMEOVER)
               - Inizializzazione centralizzata della struttura Game e random seed
               - Navigazione menu multilivello con supporto sia frecce direzionali che tasti WASD
               - Integrazione completa con sistema input multipiattaforma tramite getch_custom()
               - Macchina a stati a ciclo continuo con switch principale per transizioni controllate
               - Gestione eventi di input specifici per ogni stato con logiche dedicate
               - Chiamate coordinate alle funzioni di rendering appropriate per ogni schermata
               - Gestione uscita applicazione, reset partite e riavvio con mantenimento impostazioni
               - Coordinamento tra logica Sudoku, sistema interfaccia e gestione salvataggi
               - Timer di gioco integrato tramite time() con calcolo minuti/secondi
               - Supporto per salvataggio rapido in-game (tasto V) con conferma visuale
               - Fallback a input numerico diretto (1-3) per selezione rapida difficoltà
               - Gestione errori di caricamento con ritorno allo stato precedente
               - Transizioni di stato fluide con pulizia schermo e re-inizializzazioni appropriate
               - Controllo coerenza dati tra stati diversi e prevenzione condizioni inconsistenti
               - Punto di ingresso unificato per tutta l'applicazione con gestione risorse centralizzata
 ============================================================================
 */

// Include standard
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Include multipiattaforma
#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <termios.h>
    #include <unistd.h>
#endif

// Include personalizzati
#include "input.h"
#include "sudoku.h"

/**
 * Pulisce il terminale in modo cross-platform
 */
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}


int main() {
    srand(time(NULL));
    Game game;
    strcpy(game.gameName, "");
    game.gameState = STATE_MENU;

    int prevGameState = STATE_MENU;
  /* ========== NASCONDI CURSORE ALL'INIZIO ========== */
    hide_terminal_cursor();
    
    /* ========== GESTIONE SEGNALI PER RIPRISTINO ========== */
#ifdef _WIN32
    // Windows - usa SetConsoleCtrlHandler
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)show_terminal_cursor, TRUE);
#else
    // Unix/Linux - usa signal()
    signal(SIGINT, (void (*)(int))show_terminal_cursor);
    signal(SIGTERM, (void (*)(int))show_terminal_cursor);
#endif
    
    /* ========== RIPRISTINA CURSORE ALL'USCITA ========== */
    atexit(show_terminal_cursor);
    while (1) {
        switch (game.gameState) {
            case STATE_MENU: {
                int selected = 0;
                int menu_items = 3; // Nuova partita, Carica, Esci

                while(1) {
                    showMainMenu(selected);
                    int key = getch_custom();

                    // Navigazione menu con WASD o frecce
                    if (key == CURSOR_UP || key == 'w' || key == 'W') {
                        selected = (selected > 0) ? selected - 1 : menu_items - 1;
                    }
                    else if (key == CURSOR_DOWN || key == 's' || key == 'S') {
                        selected = (selected < menu_items - 1) ? selected + 1 : 0;
                    }
                    else if (key == CURSOR_SELECT) {
                        if(selected == 0) {
                            game.gameState = STATE_DIFFICULTY;
                            break;
                        }
                        else if(selected == 1) {
                            game.gameState = STATE_LOADMENU;
                            break;
                        }
                        else if(selected == 2) {
                            return 0;
                        }
                    }
                    else if (key == CURSOR_BACK) {
                        return 0;
                    }
                }
                break;
            }

            case STATE_DIFFICULTY: {
                int selected = 0;
                int menu_items = 3;

                while(1) {
                    showDifficultyMenu(selected);
                    int key = getch_custom();

                    // Faccio funzionare sia le frecce direzionali sia WASD
                    if (key == CURSOR_UP || key == 'w' || key == 'W') {
                        selected = (selected > 0) ? selected - 1 : menu_items - 1;
                    }
                    else if (key == CURSOR_DOWN || key == 's' || key == 'S') {
                        selected = (selected < menu_items - 1) ? selected + 1 : 0;
                    }
                    // Inserisci Invio oppure Numero per la difficoltà
                    else if (key == CURSOR_SELECT || (key >= '1' && key <= '3')) {
                        int difficulty = (key >= '1' && key <= '3') ? (key - '0') : (selected + 1);
                        newGame(&game, difficulty);
                        game.gameState = STATE_PLAYING;
                        break;
                    }
                    else if (key == CURSOR_BACK) {
                        game.gameState = STATE_MENU;
                        break;
                    }
                }
                break;
            }

            case STATE_LOADMENU: {
                int selectedSave = showDynamicLoadMenu(0);
                if (selectedSave != -1) {
                    char filename[100];
                    snprintf(filename, sizeof(filename), "save%d.txt", selectedSave);
                    loadGameOption(&game, filename);
                } else {
                    game.gameState = STATE_MENU;
                }
                break;
            }

           case STATE_PLAYING: {
    // Salva posizione cursore PRIMA di processare input
    int oldRow = game.cursorRow;
    int oldCol = game.cursorCol;
    
    // Reset flag se si rientra da altro stato
    if (prevGameState != STATE_PLAYING) {
        clearScreen();
        showGameInterface(&game);

        prevGameState = STATE_PLAYING;
    }
    
    // PROCESS INPUT
    handleGameInput(&game);
    
    // DECISIONE: cosa aggiornare?
    if (game.gameState != STATE_PLAYING) {
        // Cambio stato → prepara ridisegno completo

    }
    else if (oldRow != game.cursorRow || oldCol != game.cursorCol) {
        // Solo movimento cursore → aggiornamento parziale
        updateCursorOnly(&game, oldRow, oldCol);
    }
    else {
        // Inserimento numero/errore → ridisegno completo
        clearScreen();
        showGameInterface(&game);
    }
    
    // Salva stato corrente
    prevGameState = game.gameState;
    break;
}

            case STATE_PAUSED:
                clearScreen();
                printf("[Pausa] Premi [R] per continuare, [M] per tornare al menu.\n");
                {
                    int key = getch_custom();
                    if (key == 'r' || key == 'R') {
                        game.gameState = STATE_PLAYING;
                    } else if (key == 'm' || key == 'M') {
                        game.gameState = STATE_MENU;
                    }
                }
                break;

            case STATE_WIN:
                showWinScreen(&game);
                {
                    int key = getch_custom();
                    if (key == 'n' || key == 'N') {
                        newGame(&game, game.sudoku.difficultyLevel);
                        game.gameState = STATE_PLAYING;
                    } else if (key == 'm' || key == 'M') {
                        game.gameState = STATE_MENU;
                    }
                }
                break;

            case STATE_GAMEOVER:
                showGameOverScreen();
                {
                    int key = getch_custom();
                    if (key == 'n' || key == 'N') {
                        newGame(&game, game.sudoku.difficultyLevel);
                        game.gameState = STATE_PLAYING;
                    } else if (key == 'm' || key == 'M') {
                        game.gameState = STATE_MENU;
                    }
                }
                break;

            default:
                break;
        }
    }

    return 0;
}
