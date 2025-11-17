/*
 ============================================================================
 Name        : sudoku.h
 Author      : Leonardo, Antonio, Francesco, Michele, Vincenzo
 Descrizione : Definizioni per il Sudoku X
               - Strutture dati fondamentali (Sudoku, Game)
               - Costanti di configurazione
               - Prototipi di tutte le funzioni pubbliche
               - Enumerativi per stati di gioco e difficoltà

 ============================================================================
 */

#ifndef SUDOKU_H
#define SUDOKU_H
#include <time.h>
#include <stdbool.h>
#include <dirent.h>

// ================= LAYOUT INTERFACCIA =================
#define MARGIN_LEFT  10 			  // Spaziatura interfaccia
#define TOTAL_ROWS   25
#define INNER_WIDTH  78
#define MAX_SIZE 36
#define UNASSIGNED 0

// ================= COLORI TERMINALE =================
#define COL_BORDER   "\033[1;36m"
#define COL_CURSOR   "\033[1;33m" 	  //Giallo pastello (ANSI)
#define COL_DIAGONAL "\x1b[38;5;27m"
#define COL_RESET    "\033[0m"
#define RST          "\033[0m"

// ================= STATI DI GIOCO =================
#define STATE_MENU       0
#define STATE_DIFFICULTY 1
#define STATE_LOADMENU   2
#define STATE_PLAYING    3
#define STATE_PAUSED     4
#define STATE_WIN        5
#define STATE_GAMEOVER   6

// ================= STRUCT SUDOKU =================

typedef struct {
    int grid[MAX_SIZE][MAX_SIZE];     // Griglia di gioco
    int solnGrid[MAX_SIZE][MAX_SIZE];
    int gridPos[MAX_SIZE * MAX_SIZE];
    int guessNum[MAX_SIZE];
    int difficultyLevel;
    int grid_status;
    int size;  						  // Dimensione griglia (es. 9 per 9x9)
    int boxSize;                      // Dimensione sottogriglie (es. 3)
} Sudoku;

// ================= STRUCT GAME =================

typedef struct {
    Sudoku sudoku;
    int gameState;
    int cursorRow, cursorCol;
    int score;
    int errors;
    time_t startTime;
    char gameName[64];
} Game;

// ================= FUNZIONI SUDOKU =================

void initSudoku(Sudoku* s, int boxSize);
int solveGrid(Sudoku* s);
void generatePuzzle(Sudoku* s, int difficultyLevel);
int findUnassignedLocation(Sudoku* s, int* row, int* col);
void printGrid(Sudoku* s);
int isComplete(Sudoku* s);
void printLineWithBordersCentered(const char* text);
int isSafe(Sudoku* s, int row, int col, int num);
int saveGame(Sudoku* s, const char* filename, int cursorRow, int cursorCol, int errors, int score, const char* gameName);
int loadGame(Sudoku* s, const char* filename, int* cursorRow, int* cursorCol, int* errors, int* score, char* gameNameOut);

// ================= FUNZIONI SALVATAGGI ==============================

int countSaveFiles();
int showDynamicLoadMenu(int selected);
void handleDynamicLoadInput(Game* game);

// ================= FUNZIONI INTERFACCIA & GIOCO =================

void clearScreen();
void showMainMenu(int selected);
void showDifficultyMenu(int selected);
void showLoadMenu();
void showGameOverScreen();
void showGameInterface(Game* game);
void showWinScreen(Game* game);
void newGame(Game* game, int difficultyLevel);
void loadGameOption(Game* game, const char* filename);
void handleGameInput(Game* game);
int getch_custom();
void deleteSavedGameInterface();

#endif
