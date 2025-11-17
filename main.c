/*
 ============================================================================
 Name        : main.c
 Author      : Leonardo, Antonio, Francesco, Michele, Vincenzo
 Descrizione : Loop principale di gioco e gestione stati
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

            case STATE_PLAYING:
                showGameInterface(&game);
                handleGameInput(&game);
                break;

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
