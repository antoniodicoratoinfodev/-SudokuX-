/*
 ============================================================================
 Name        : sudoku.c
 Descrizione : Implementazione delle funzionalità logiche del Sudoku X
               - Generazione e risoluzione di griglie
               - Gestione regole del Sudoku con vincoli diagonali
               - Sistema di salvataggio/caricamento partite
               - Renderizzazione interfaccia testuale
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "sudoku.h"
#include <string.h>
#include <unistd.h>
#include <ctype.h>

/*  questo: */
#include "input.h"  // INCLUDA SEMPRE input.h - FUORI dal #ifdef

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#endif
/* ========== FUNZIONE HELPER PER TROVARE NUMERO DISPONIBILE ========== */

/**
 * Trova il primo numero di salvataggio disponibile (1-99)
 * Scansiona i file save1.txt, save2.txt, ... e ritorna il primo buco
 * Ritorna 0 se tutti i numeri 1-99 sono occupati
 */
static int findAvailableSaveNumber() {
    int num = 1;
    char filename[100];
    FILE* testFile;
    
    while (num <= 99) {
        snprintf(filename, sizeof(filename), "save%d.txt", num);
        testFile = fopen(filename, "r");
        
        if (testFile) {
            // File esiste, prova il successivo
            fclose(testFile);
            num = num + 1;
        } else {
            // File non esiste, numero disponibile
            return num;
        }
    }
    
    // Tutti i numeri 1-99 occupati
    return 0;
}
/* ========== FUNZIONI DI UTILITÀ PER L'INTERFACCIA ========== */

/**
 * Disegna una singola linea orizzontale per i bordi dell'interfaccia
 */
void drawBorderLine() {
    int i = 0;
    while (i < MARGIN_LEFT) {
        printf(" ");
        i = i + 1;
    }
    printf("+");
    i = 0;
    while (i < INNER_WIDTH) {
        printf("-");
        i = i + 1;
    }
    printf("+\n");
}

/**
 * Stampa testo centrato tra bordi verticali, senza gestione colori ANSI.
 */
void printCenteredLine(const char* text) {
    int i = 0;
    while (i < MARGIN_LEFT) {
        printf(" ");
        i = i + 1;
    }
    printf("|");

    int text_len = strlen(text);
    int left_spaces = (INNER_WIDTH - text_len) / 2;
    int right_spaces = INNER_WIDTH - text_len - left_spaces;

    i = 0;
    while (i < left_spaces) {
        printf(" ");
        i = i + 1;
    }
    printf("%s", text);
    i = 0;
    while (i < right_spaces) {
        printf(" ");
        i = i + 1;
    }
    printf("|\n");
}

/**
 * Disegna un bordo completo (superiore/inferiore)
 */
void drawFullBorder() {
    int i = 0;
    while (i < MARGIN_LEFT) {
        printf(" ");
        i = i + 1;
    }
    printf("+");
    i = 0;
    while (i < INNER_WIDTH) {
        printf("-");
        i = i + 1;
    }
    printf("+\n");
}

/**
 * Come printCenteredLine(), ma ignora i codici ANSI per
 * calcolare correttamente la lunghezza del testo.
 */
void printPerfectCentered(const char* text) {
    int i = 0;
    while (i < MARGIN_LEFT) {
        printf(" ");
        i = i + 1;
    }
    printf("|");

    int visible_len = 0;
    const char *p = text;
    while (*p) {
        if (*p == '\033') {
            while (*p && *p != 'm') {
                p = p + 1;
            }
            if (*p) {
                p = p + 1;
            }
            continue;
        }
        visible_len = visible_len + 1;
        p = p + 1;
    }

    int left_spaces = (INNER_WIDTH - visible_len) / 2;
    int right_spaces = INNER_WIDTH - visible_len - left_spaces;

    i = 0;
    while (i < left_spaces) {
        printf(" ");
        i = i + 1;
    }
    printf("%s", text);
    i = 0;
    while (i < right_spaces) {
        printf(" ");
        i = i + 1;
    }
    printf("|\n");
}

/**
 *  Conta i file di salvataggio nel formato saveX.txt.
 */
int countSaveFiles() {
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    dir = opendir(".");
    if (dir == NULL) {
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "save", 4) == 0 &&
            strstr(entry->d_name, ".txt") != NULL) {
            count = count + 1;
        }
    }

    closedir(dir);
    return count;
}

/**
* Mostra un menu interattivo per caricare un salvataggio, con opzione di eliminazione.
*/
int showDynamicLoadMenu(int selected) {
    int foundSaves = 0;
    int saveNumbers[9] = {0};
    char saveNames[9][64];
    char buffer[100];
    int i = 1;

    // Scansione dei file di salvataggio
    while (i <= 99 && foundSaves < 9) {
        snprintf(buffer, sizeof(buffer), "save%d.txt", i);
        FILE* file = fopen(buffer, "r");
        if (file) {
            if (fgets(saveNames[foundSaves], sizeof(saveNames[foundSaves]), file)) {
                size_t len = strlen(saveNames[foundSaves]);
                if (len > 0 && saveNames[foundSaves][len - 1] == '\n') {
                    saveNames[foundSaves][len - 1] = '\0';
                }
            }
            fclose(file);
            saveNumbers[foundSaves] = i;
            foundSaves = foundSaves + 1;
        }
        i = i + 1;
    }

    // Caso nessun salvataggio trovato
    if (foundSaves == 0) {
        while (1) {
            clearScreen();

            drawFullBorder();
            printPerfectCentered("CARICA PARTITA");
            drawFullBorder();

            i = 0;
            while (i < 8) {
                printPerfectCentered("");
                i = i + 1;
            }

            printPerfectCentered("Nessun salvataggio trovato!");
            printPerfectCentered("");

            char instructions[150];
            snprintf(instructions, sizeof(instructions),
                "Premi %sINVIO%s o %sESC%s per tornare indietro",
                COL_CURSOR, COL_RESET, COL_CURSOR, COL_RESET);
            printPerfectCentered(instructions);

            i = 0;
            while (i < 7) {
                printPerfectCentered("");
                i = i + 1;
            }

            drawFullBorder();

            int key = getch_custom();
            if (key == CURSOR_SELECT || key == CURSOR_BACK) {
                return -1;
            }
        }
    }

    // Caso normale con salvataggi disponibili
    while (1) {
        clearScreen();

        drawFullBorder();
        printPerfectCentered("CARICA PARTITA");
        drawFullBorder();
        printPerfectCentered("");

        // Lista salvataggi
        i = 0;
        while (i < foundSaves) {
            char line[100];
            snprintf(line, sizeof(line), "%s (save%d.txt)", saveNames[i], saveNumbers[i]);

            if (i == selected) {
                char selectedLine[120];
                snprintf(selectedLine, sizeof(selectedLine), "%s> %s <%s",
                    COL_CURSOR, line, COL_RESET);
                printPerfectCentered(selectedLine);
            } else {
                printPerfectCentered(line);
            }
            i = i + 1;
        }

        // Riempimento spazi vuoti
        i = foundSaves;
        while (i < 9) {
            printPerfectCentered("");
            i = i + 1;
        }

        // Spaziatura prima delle istruzioni
        i = 0;
        while (i < 3) {
            printPerfectCentered("");
            i = i + 1;
        }

        // Istruzioni con opzione eliminazione
        char instructions[200];
        snprintf(instructions, sizeof(instructions),
            "Usa %sW/S%s per muoverti, %sINVIO%s per selezionare, %sD%s per eliminare, %sESC%s per tornare",
            COL_CURSOR, COL_RESET, COL_CURSOR, COL_RESET, COL_CURSOR, COL_RESET, COL_CURSOR, COL_RESET);
        printPerfectCentered(instructions);

        drawFullBorder();

        // Gestione input
        int key = getch_custom();
        switch (key) {
            case CURSOR_UP: case 'w': case 'W':
                if (selected > 0) {
                    selected = selected - 1;
                }
                break;

            case CURSOR_DOWN: case 's': case 'S':
                if (selected < foundSaves - 1) {
                    selected = selected + 1;
                }
                break;

            case CURSOR_SELECT: case '\n':
                return saveNumbers[selected];

            case CURSOR_BACK: case 27: // ESC
                return -1;

            case 'd': case 'D': // Eliminazione
                clearScreen();
                drawFullBorder();
                printPerfectCentered("CONFERMA ELIMINAZIONE");
                drawFullBorder();

                char confirmMsg[120];
                snprintf(confirmMsg, sizeof(confirmMsg), "%s%s (save%d.txt)%s",
                    COL_CURSOR, saveNames[selected], saveNumbers[selected], COL_RESET);
                printPerfectCentered(confirmMsg);
                printPerfectCentered("");
                printPerfectCentered("[Y] Conferma  [N] Annulla");
                drawFullBorder();

                if (tolower(getch_custom()) == 'y') {
                    char filename[100];
                    snprintf(filename, sizeof(filename), "save%d.txt", saveNumbers[selected]);

                    if (remove(filename) == 0) {
                        i = selected;
                        while (i < foundSaves - 1) {
                            saveNumbers[i] = saveNumbers[i + 1];
                            strcpy(saveNames[i], saveNames[i + 1]);
                            i = i + 1;
                        }
                        foundSaves = foundSaves - 1;

                        if (foundSaves == 0) {
                            return -1;
                        }
                        if (selected >= foundSaves) {
                            selected = foundSaves - 1;
                        }
                    }
                }
                break;
        }
    }
}

/**
* Gestione input per il menu di caricamento
*/
void handleDynamicLoadInput(Game* game) {
    int key = getch_custom();

    if (key >= '1' && key <= '9') {
        int choice = key - '0';
        int foundSaves = 0;
        char filename[32];
        int saveNumbers[9];
        int i = 1;

        while (i <= 99 && foundSaves < 9) {
            snprintf(filename, sizeof(filename), "save%d.txt", i);
            FILE* f = fopen(filename, "r");
            if (f) {
                fclose(f);
                saveNumbers[foundSaves] = i;
                foundSaves = foundSaves + 1;
            }
            i = i + 1;
        }

        if (choice <= foundSaves) {
            int realSaveNum = saveNumbers[choice - 1];
            snprintf(filename, sizeof(filename), "save%d.txt", realSaveNum);
            loadGameOption(game, filename);
        } else {
            clearScreen();
            printf("\nSalvataggio non trovato!\n");
            sleep(1);
        }
    } else if (key == 't' || key == 'T') {
        game->gameState = STATE_MENU;
    }
}

/**
 * Inizializza una griglia vuota e imposta la dimensione
 */
void initSudoku(Sudoku* s, int boxSize) {
    s->boxSize = boxSize;
    s->size = boxSize * boxSize;
    s->difficultyLevel = 0;
    s->grid_status = 1;

    srand(time(NULL));

    int i = 0;
    while (i < s->size) {
        int j = 0;
        while (j < s->size) {
            s->grid[i][j] = UNASSIGNED;
            j = j + 1;
        }
        i = i + 1;
    }
}

/**
 * Genera un numero casuale
 */
int genRandNum(int maxLimit) {
    return rand() % maxLimit;
}

/**
 * Verifica se un numero può essere inserito in una cella
 */
int isSafe(Sudoku* s, int row, int col, int num) {
    int size = s->size;
    int box = s->boxSize;
    int i = 0;

    // Verifica riga e colonna
    while (i < size) {
        if (s->grid[row][i] == num || s->grid[i][col] == num) {
            return 0;
        }
        i = i + 1;
    }

    // Verifica box
    int startRow = row - (row % box);
    int startCol = col - (col % box);
    i = 0;
    while (i < box) {
        int j = 0;
        while (j < box) {
            if (s->grid[startRow + i][startCol + j] == num) {
                return 0;
            }
            j = j + 1;
        }
        i = i + 1;
    }

    // Verifica diagonale principale
    if (row == col) {
        i = 0;
        while (i < size) {
            if (s->grid[i][i] == num) {
                return 0;
            }
            i = i + 1;
        }
    }

    // Verifica diagonale secondaria
    if (row + col == size - 1) {
        i = 0;
        while (i < size) {
            if (s->grid[i][size - 1 - i] == num) {
                return 0;
            }
            i = i + 1;
        }
    }

    return 1;
}

/**
* Controlla la validità delle diagonali principali
*/
int isValidDiagonals(Sudoku* s) {
    int size = s->size;
    int diag1[MAX_SIZE] = {0};
    int diag2[MAX_SIZE] = {0};
    int i = 0;

    while (i < size) {
        int val1 = s->grid[i][i];
        int val2 = s->grid[i][size - 1 - i];

        if (val1 != 0) {
            if (diag1[val1 - 1]) {
                return 0;
            }
            diag1[val1 - 1] = 1;
        }
        if (val2 != 0) {
            if (diag2[val2 - 1]) {
                return 0;
            }
            diag2[val2 - 1] = 1;
        }
        i = i + 1;
    }

    return 1;
}

/**
* Trova la prima cella vuota nella griglia
 */
int findUnassignedLocation(Sudoku* s, int* row, int* col) {
    int i = 0;
    while (i < s->size) {
        int j = 0;
        while (j < s->size) {
            if (s->grid[i][j] == UNASSIGNED) {
                *row = i;
                *col = j;
                return true;
            }
            j = j + 1;
        }
        i = i + 1;
    }
    return false;
}

int solveGrid(Sudoku* s) {
    int row, col;
    if (findUnassignedLocation(s, &row, &col) == 0) {
        return 1;
    }

    // Crea un array dei numeri e mescolalo
    int nums[s->size];
    int i = 0;
    while (i < s->size) {
        nums[i] = i + 1;
        i = i + 1;
    }

    // Mescola
    i = 0;
    while (i < s->size) {
        int randIndex = rand() % s->size;
        int temp = nums[i];
        nums[i] = nums[randIndex];
        nums[randIndex] = temp;
        i = i + 1;
    }

    // Prova i numeri nell'ordine randomizzato
    i = 0;
    while (i < s->size) {
        int num = nums[i];
        if (isSafe(s, row, col, num)) {
            s->grid[row][col] = num;

            if (solveGrid(s)) {
                return 1;
            }

            s->grid[row][col] = UNASSIGNED;
        }
        i = i + 1;
    }

    return 0;
}

void generatePuzzle(Sudoku* s, int difficultyLevel) {
    int totalCells = s->size * s->size;
    int toRemove = 0;

    // Imposta quante celle rimuovere in base alla difficoltà
    switch (difficultyLevel) {
        case 1: // Facile ~33%
            toRemove = totalCells / 3;
            break;
        case 2: // Intermedio ~50%
            toRemove = totalCells / 2;
            break;
        case 3: // Difficile ~75%
            toRemove = (3 * totalCells) / 4;
            break;
        default:
            toRemove = totalCells / 3;
            break;
    }

    while (toRemove > 0) {
        int i = genRandNum(s->size);
        int j = genRandNum(s->size);
        if (s->grid[i][j] != UNASSIGNED) {
            s->grid[i][j] = UNASSIGNED;
            toRemove = toRemove - 1;
        }
    }
}

void printGrid(Sudoku* s) {
    printf("\n");
    int i = 0;
    while (i < s->size) {
        if (i % s->boxSize == 0 && i > 0) {
            int k = 0;
            while (k < s->size * 2 + s->boxSize - 1) {
                printf("-");
                k = k + 1;
            }
            printf("\n");
        }

        int j = 0;
        while (j < s->size) {
            if (j % s->boxSize == 0 && j > 0) {
                printf("| ");
            }

            if (s->grid[i][j] == UNASSIGNED) {
                printf(". ");
            } else {
                printf("%d ", s->grid[i][j]);
            }
            j = j + 1;
        }
        printf("\n");
        i = i + 1;
    }
    printf("\n");
}

/**
* Verifica se la griglia è risolta correttamente
*/
int isComplete(Sudoku* s) {
    int i = 0;
    while (i < s->size) {
        int j = 0;
        while (j < s->size) {
            if (s->grid[i][j] == UNASSIGNED) {
                return 0;
            }
            j = j + 1;
        }
        i = i + 1;
    }
    return isValidDiagonals(s);
}

int saveGame(Sudoku* s, const char* filename, int cursorRow, int cursorCol, int errors, int score, const char* gameName) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        return 0;
    }

    fprintf(file, "%s\n", gameName);
    fprintf(file, "%d %d\n", s->size, s->boxSize);
    fprintf(file, "%d %d %d %d\n", cursorRow, cursorCol, errors, score);

    int i = 0;
    while (i < s->size) {
        int j = 0;
        while (j < s->size) {
            fprintf(file, "%d ", s->grid[i][j]);
            j = j + 1;
        }
        fprintf(file, "\n");
        i = i + 1;
    }

    fclose(file);
    return 1;
}

int loadGame(Sudoku* s, const char* filename, int* cursorRow, int* cursorCol, int* errors, int* score, char* gameNameOut) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return 0;
    }

    if (!fgets(gameNameOut, 32, file)) {
        fclose(file);
        return 0;
    }

    size_t len = strlen(gameNameOut);
    if (len > 0 && gameNameOut[len - 1] == '\n') {
        gameNameOut[len - 1] = '\0';
    }

    int size, boxSize;
    if (fscanf(file, "%d %d", &size, &boxSize) != 2) {
        fclose(file);
        return 0;
    }

    initSudoku(s, boxSize);

    if (fscanf(file, "%d %d %d %d", cursorRow, cursorCol, errors, score) != 4) {
        fclose(file);
        return 0;
    }

    int i = 0;
    while (i < s->size) {
        int j = 0;
        while (j < s->size) {
            if (fscanf(file, "%d", &s->grid[i][j]) != 1) {
                fclose(file);
                return 0;
            }
            j = j + 1;
        }
        i = i + 1;
    }

    fclose(file);
    return 1;
}

// =========================== FUNZIONI PER INTERFACCIA ===========================

/**
* Versione semplificata di printPerfectCentered() senza gestione colori
*/
void printLineWithBordersCentered(const char* text) {
    int text_len = (int)strlen(text);
    int left_spaces = (INNER_WIDTH - text_len) / 2;
    int right_spaces = INNER_WIDTH - text_len - left_spaces;
    int i = 0;

    while (i < MARGIN_LEFT) {
        printf(" ");
        i = i + 1;
    }
    printf("|");

    i = 0;
    while (i < left_spaces) {
        printf(" ");
        i = i + 1;
    }
    printf("%s", text);
    i = 0;
    while (i < right_spaces) {
        printf(" ");
        i = i + 1;
    }
    printf("|\n");
}

void showDifficultyMenu(int selected) {
    clearScreen();

    drawFullBorder();
    printPerfectCentered("SELEZIONA DIFFICOLTA'");
    drawFullBorder();
    printPerfectCentered("");

    const char* difficulties[] = {"[1] Facile", "[2] Intermedio", "[3] Difficile"};
    int i = 0;
    while (i < 3) {
        char line[100];
        if (i == selected) {
            snprintf(line, sizeof(line), "%s> %s <%s", COL_CURSOR, difficulties[i], COL_RESET);
        } else {
            snprintf(line, sizeof(line), "  %s  ", difficulties[i]);
        }
        printPerfectCentered(line);
        i = i + 1;
    }

    i = 0;
    while (i < 12) {
        printPerfectCentered("");
        i = i + 1;
    }

    char instructions[150];
    snprintf(instructions, sizeof(instructions),
        "Usa %sW/S%s per muoverti, %sINVIO%s per selezionare, %sESC%s per tornare indietro",
        COL_CURSOR, COL_RESET, COL_CURSOR, COL_RESET, COL_CURSOR, COL_RESET);
    printPerfectCentered(instructions);

    drawFullBorder();
}

void showGameOverScreen() {
    clearScreen();

    int i = 0;
    while (i < MARGIN_LEFT) {
        printf(" ");
        i = i + 1;
    }
    printf("+------------------------------------------------------------------------------+\n");

    printLineWithBordersCentered("SUDOKU X");

    i = 0;
    while (i < MARGIN_LEFT) {
        printf(" ");
        i = i + 1;
    }
    printf("+------------------------------------------------------------------------------+\n");

    const char* lines[] = {
        "Game Over!",
        "[N] Nuova Partita    [M] Menu Iniziale"
    };
    int menu_lines = sizeof(lines) / sizeof(lines[0]);
    int total_menu_lines = menu_lines + (menu_lines - 1);
    int available_lines = TOTAL_ROWS - 4;
    int empty_lines_before = (available_lines - total_menu_lines) / 2;

    i = 0;
    while (i < empty_lines_before) {
        int j = 0;
        while (j < MARGIN_LEFT) {
            printf(" ");
            j = j + 1;
        }
        printf("|");
        int k = 0;
        while (k < INNER_WIDTH) {
            printf(" ");
            k = k + 1;
        }
        printf("|\n");
        i = i + 1;
    }

    i = 0;
    while (i < menu_lines) {
        printLineWithBordersCentered(lines[i]);
        if (i < menu_lines - 1) {
            int j = 0;
            while (j < MARGIN_LEFT) {
                printf(" ");
                j = j + 1;
            }
            printf("|");
            int k = 0;
            while (k < INNER_WIDTH) {
                printf(" ");
                k = k + 1;
            }
            printf("|\n");
        }
        i = i + 1;
    }

    int remaining_lines = available_lines - empty_lines_before - total_menu_lines;
    i = 0;
    while (i < remaining_lines) {
        int j = 0;
        while (j < MARGIN_LEFT) {
            printf(" ");
            j = j + 1;
        }
        printf("|");
        int k = 0;
        while (k < INNER_WIDTH) {
            printf(" ");
            k = k + 1;
        }
        printf("|\n");
        i = i + 1;
    }

    i = 0;
    while (i < MARGIN_LEFT) {
        printf(" ");
        i = i + 1;
    }
    printf("+------------------------------------------------------------------------------+\n");
}

void showMainMenu(int selected) {
    clearScreen();

    drawFullBorder();
    printPerfectCentered("SUDOKU X");
    drawFullBorder();
    printPerfectCentered("");

    const char* items[] = {"Nuova Partita", "Carica Partita", "Esci"};
    int i = 0;
    while (i < 3) {
        char line[100];
        if (i == selected) {
            snprintf(line, sizeof(line), "%s> %s <%s", COL_CURSOR, items[i], COL_RESET);
        } else {
            snprintf(line, sizeof(line), "  %s  ", items[i]);
        }
        printPerfectCentered(line);
        i = i + 1;
    }

    i = 0;
    while (i < 12) {
        printPerfectCentered("");
        i = i + 1;
    }

    char instructions[150];
    snprintf(instructions, sizeof(instructions),
        "Usa %sW/S%s per muoverti, %sINVIO%s per selezionare, %sESC%s per uscire",
        COL_CURSOR, COL_RESET, COL_CURSOR, COL_RESET, COL_CURSOR, COL_RESET);
    printPerfectCentered(instructions);

    drawFullBorder();
}

void showLoadMenu() {
    clearScreen();
    int i = 0;
    while (i < MARGIN_LEFT) {
        printf(" ");
        i = i + 1;
    }
    printf("+------------------------------------------------------------------------------+"RST"\n");

    printLineWithBordersCentered("CARICA PARTITA");

    i = 0;
    while (i < MARGIN_LEFT) {
        printf(" ");
        i = i + 1;
    }
    printf("+------------------------------------------------------------------------------+"RST"\n");

    const char* lines[] = {
        "[1] Salvataggio 1",
        "[2] Salvataggio 2",
        "[3] Salvataggio 3",
        "[T] Torna al menu principale"
    };
    i = 0;
    while (i < 4) {
        int j = 0;
        while (j < MARGIN_LEFT) {
            printf(" ");
            j = j + 1;
        }
        printf("|"RST" %-76s ""|"RST"\n", lines[i]);
        i = i + 1;
    }

    i = 0;
    while (i < 18) {
        int j = 0;
        while (j < MARGIN_LEFT) {
            printf(" ");
            j = j + 1;
        }
        printf("|"RST"                                                                              ""|"RST"\n");
        i = i + 1;
    }

    i = 0;
    while (i < MARGIN_LEFT) {
        printf(" ");
        i = i + 1;
    }
    printf("+------------------------------------------------------------------------------+"RST"\n");
}

void showGameInterface(Game* game) {
    clearScreen();

    Sudoku* s = &game->sudoku;
    const char* diagonalColor = COL_DIAGONAL;

    int elapsed = (int)(time(NULL) - game->startTime);
    int minutes = elapsed / 60;
    int seconds = elapsed % 60;

    int cellWidth = 3;
    int leftPadding = 12;
    int baseRightPadding = 25;
    int gridWidth = s->size * cellWidth + (s->size / s->boxSize - 1);
    int totalWidth = leftPadding + 1 + gridWidth + 1 + baseRightPadding + 1;

    // Bordo superiore
    int i = 0;
    while (i < MARGIN_LEFT) {
        printf(" ");
        i = i + 1;
    }
    printf("+");
    i = 0;
    while (i < totalWidth - 2) {
        printf("-");
        i = i + 1;
    }
    printf("+\n");

    // Titolo centrato
    i = 0;
    while (i < MARGIN_LEFT) {
        printf(" ");
        i = i + 1;
    }
    printf("|");
    int left = (totalWidth - 2 - 8) / 2;
    int right = totalWidth - 2 - 8 - left;
    i = 0;
    while (i < left) {
        printf(" ");
        i = i + 1;
    }
    printf("Sudoku ");
    printf("%sX%s", diagonalColor ? diagonalColor : "", COL_RESET);
    i = 0;
    while (i < right) {
        printf(" ");
        i = i + 1;
    }
    printf("|\n");

    // Riga vuota
    i = 0;
    while (i < MARGIN_LEFT) {
        printf(" ");
        i = i + 1;
    }
    printf("|");
    i = 0;
    while (i < totalWidth - 2) {
        printf(" ");
        i = i + 1;
    }
    printf("|\n");

    // Riga separatrice superiore griglia
    i = 0;
    while (i < MARGIN_LEFT) {
        printf(" ");
        i = i + 1;
    }
    printf("|");
    int k = 0;
    while (k < leftPadding - 1) {
        printf(" ");
        k = k + 1;
    }
    printf("+");
    int b = 0;
    while (b < s->size) {
        printf("---");
        if ((b + 1) % s->boxSize == 0 && b < s->size - 1) printf("+");
        b = b + 1;
    }
    printf("+");
    k = 0;
    while (k < baseRightPadding) {
        printf(" ");
        k = k + 1;
    }
    printf("|\n");

    // Riga per riga
    i = 0;
    while (i < s->size) {
        int m = 0;
        while (m < MARGIN_LEFT) {
            printf(" ");
            m = m + 1;
        }
        printf("|");
        k = 0;
        while (k < leftPadding - 1) {
            printf(" ");
            k = k + 1;
        }
        printf("|");

        int j = 0;
        while (j < s->size) {
            int isDiagonal = (i == j) || (i + j == s->size - 1);
            int isCursor = (i == game->cursorRow && j == game->cursorCol);
            int val = s->grid[i][j];

            const char* colorPrefix = "";
            const char* colorSuffix = "";

            if (isCursor) {
                colorPrefix = COL_CURSOR;
                colorSuffix = COL_RESET;
            } else if (isDiagonal && diagonalColor) {
                colorPrefix = diagonalColor;
                colorSuffix = COL_RESET;
            }

            if (isCursor) {
                if (val == UNASSIGNED)
                    printf("%s[%c]%s", colorPrefix, ' ', colorSuffix);
                else
                    printf("%s[%d]%s", colorPrefix, val, colorSuffix);
            } else {
                if (val == UNASSIGNED)
                    printf("%s . %s", colorPrefix, colorSuffix);
                else
                    printf("%s %d %s", colorPrefix, val, colorSuffix);
            }

            if ((j + 1) % s->boxSize == 0 && j < s->size - 1) printf("|");
            j = j + 1;
        }

        printf("|");

        // Lato destro dinamico
        const char* rightText = "";
        if (i == 1) rightText = "Inserisci i numeri:";
        else if (i == 3) rightText = "  [1]   [2]   [3]";
        else if (i == 4) rightText = "  [4]   [5]   [6]";
        else if (i == 5) rightText = "  [7]   [8]   [9]";

        char buffer[64];
        snprintf(buffer, sizeof(buffer), " %-*s", baseRightPadding - 1, rightText);
        printf("%s|\n", buffer);

        if ((i + 1) % s->boxSize == 0 && i < s->size - 1) {
            m = 0;
            while (m < MARGIN_LEFT) {
                printf(" ");
                m = m + 1;
            }
            printf("|");
            k = 0;
            while (k < leftPadding - 1) {
                printf(" ");
                k = k + 1;
            }
            printf("+");
            b = 0;
            while (b < s->size) {
                printf("---");
                if ((b + 1) % s->boxSize == 0 && b < s->size - 1) printf("+");
                b = b + 1;
            }
            printf("+");
            k = 0;
            while (k < baseRightPadding) {
                printf(" ");
                k = k + 1;
            }
            printf("|\n");
        }
        i = i + 1;
    }

    // Riga finale della griglia
    int m = 0;
    while (m < MARGIN_LEFT) {
        printf(" ");
        m = m + 1;
    }
    printf("|");
    k = 0;
    while (k < leftPadding - 1) {
        printf(" ");
        k = k + 1;
    }
    printf("+");
    b = 0;
    while (b < s->size) {
        printf("---");
        if ((b + 1) % s->boxSize == 0 && b < s->size - 1) printf("+");
        b = b + 1;
    }
    printf("+");
    k = 0;
    while (k < baseRightPadding) {
        printf(" ");
        k = k + 1;
    }
    printf("|\n");

    // Riga vuota
    m = 0;
    while (m < MARGIN_LEFT) {
        printf(" ");
        m = m + 1;
    }
    printf("|");
    i = 0;
    while (i < totalWidth - 2) {
        printf(" ");
        i = i + 1;
    }
    printf("|\n");

    // Comandi
    const char* commands[] = {
        " Comandi:",
        " sopra  [W]            destra  [D]            cancella numero [0]",
        " sotto  [S]            sinistra [A]",
        " pausa  [P]            salva [V]   "
    };
    i = 0;
    while (i < 4) {
        m = 0;
        while (m < MARGIN_LEFT) {
            printf(" ");
            m = m + 1;
        }
        printf("|");
        printf("%-*s", totalWidth - 2, commands[i]);
        printf("|\n");
        i = i + 1;
    }

    // Riga vuota
    m = 0;
    while (m < MARGIN_LEFT) {
        printf(" ");
        m = m + 1;
    }
    printf("|");
    i = 0;
    while (i < totalWidth - 2) {
        printf(" ");
        i = i + 1;
    }
    printf("|\n");

    // Footer con nome partita
    m = 0;
    while (m < MARGIN_LEFT) {
        printf(" ");
        m = m + 1;
    }
    printf("|");

    // Calcolo lunghezza visibile senza escape ANSI
    int visibleLen = (int)strlen(game->gameName) + strlen("              Tempo: 00:00  Errori:  0") + 1;
    printf(" %s%s%s              Tempo: %02d:%02d  Errori: %2d", COL_DIAGONAL, game->gameName, COL_RESET, minutes, seconds, game->errors);

    i = 0;
    while (i < totalWidth - 2 - visibleLen) {
        printf(" ");
        i = i + 1;
    }
    printf("|\n");

    // Bordo finale
    m = 0;
    while (m < MARGIN_LEFT) {
        printf(" ");
        m = m + 1;
    }
    printf("+");
    i = 0;
    while (i < totalWidth - 2) {
        printf("-");
        i = i + 1;
    }
    printf("+\n");
}

void showWinScreen(Game* game) {
    clearScreen();
    int elapsed = (int)(time(NULL) - game->startTime);
    int minutes = elapsed / 60;
    int seconds = elapsed % 60;

    // Cornice superiore
    drawFullBorder();
    printPerfectCentered("HAI VINTO!");
    drawFullBorder();
    printPerfectCentered("");

    // Contenuto centrato
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "Tempo impiegato: %02d:%02d", minutes, seconds);
    printPerfectCentered(timeStr);

    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr), "Punteggio: %d", game->score);
    printPerfectCentered(scoreStr);

    char errorsStr[32];
    snprintf(errorsStr, sizeof(errorsStr), "Errori commessi: %d", game->errors);
    printPerfectCentered(errorsStr);

    // Spaziatura
    int i = 0;
    while (i < 8) {
        printPerfectCentered("");
        i = i + 1;
    }

    // Opzioni menu - centrate con colori
    char menuLine[64];
    snprintf(menuLine, sizeof(menuLine), "%s[N]%s Nuova Partita    %s[M]%s Menu Iniziale",
             COL_CURSOR, COL_RESET, COL_CURSOR, COL_RESET);
    printPerfectCentered(menuLine);

    // Spaziatura inferiore
    i = 0;
    while (i < 7) {
        printPerfectCentered("");
        i = i + 1;
    }

    // Cornice inferiore
    drawFullBorder();
}

void newGame(Game* game, int difficultyLevel) {
    // Trova il primo numero di salvataggio disponibile
    int nextNum = findAvailableSaveNumber();
    
    if (nextNum == 0) {
        // Caso limite: tutti i numeri 1-99 occupati
        // Usa un nome fisso che non corrisponde a nessun file
        snprintf(game->gameName, sizeof(game->gameName), "Partita Temporanea");
    } else {
        // Nome standard con numero disponibile
        snprintf(game->gameName, sizeof(game->gameName), "Partita %d", nextNum);
    }
    
    int boxSize = 3;
    
    // Inizializza griglia vuota
    initSudoku(&game->sudoku, boxSize);
    
    // Genera soluzione completa
    solveGrid(&game->sudoku);
    
    // Imposta difficoltà e rimuovi celle
    game->sudoku.difficultyLevel = difficultyLevel;
    generatePuzzle(&game->sudoku, difficultyLevel);
    
    // Inizializza stato di gioco
    game->cursorRow = 0;
    game->cursorCol = 0;
    game->errors = 0;
    game->score = 0;
    game->startTime = time(NULL);
    game->gameState = STATE_PLAYING;
}
/**
* Carica una partita da file e passa allo stato PLAYING
*/
void loadGameOption(Game* game, const char* filename) {
    if (loadGame(&game->sudoku, filename, &game->cursorRow, &game->cursorCol, &game->errors, &game->score, game->gameName)) {
        game->startTime = time(NULL);
        game->gameState = STATE_PLAYING;
    } else {
        printf("Errore nel caricamento della partita.\n");
    }
}

/**
* Gestisce tutti gli input durante il gioco
*/
void handleGameInput(Game* game) {
    int key = getch_custom();

    switch (key) {
        case 'w': case 'W':
            if (game->cursorRow > 0) game->cursorRow = game->cursorRow - 1;
            break;
        case 's': case 'S':
            if (game->cursorRow < game->sudoku.size - 1) game->cursorRow = game->cursorRow + 1;
            break;
        case 'a': case 'A':
            if (game->cursorCol > 0) game->cursorCol = game->cursorCol - 1;
            break;
        case 'd': case 'D':
            if (game->cursorCol < game->sudoku.size - 1) game->cursorCol = game->cursorCol + 1;
            break;

        case '0':
            game->sudoku.grid[game->cursorRow][game->cursorCol] = UNASSIGNED;
            break;

        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9': {
            int num = key - '0';
            if (num <= game->sudoku.size) {
                if (isSafe(&game->sudoku, game->cursorRow, game->cursorCol, num)) {
                    game->sudoku.grid[game->cursorRow][game->cursorCol] = num;
                    game->score = game->score + 10;

                    if (isComplete(&game->sudoku)) {
                        game->gameState = STATE_WIN;
                    }
                } else {
                    game->errors = game->errors + 1;
                    game->score = game->score - 5;

                    if (game->errors >= 5) {
                        game->gameState = STATE_GAMEOVER;
                    }
                }
            }
            break;
        }

        case 'p': case 'P':
            game->gameState = STATE_PAUSED;
            break;

        case 'v': case 'V': {
    // Verifica se è una partita temporanea (non salvabile)
    if (strstr(game->gameName, "Temporanea") != NULL) {
        clearScreen();
        printf("\n[!] ERRORE: Partita temporanea non salvabile!\n");
        printf("    Hai raggiunto il limite di 99 salvataggi.\n");
        printf("    Elimina alcuni salvataggi per liberare spazio.\n");
        printf("\n    Premi un tasto per continuare...\n");
        getch_custom();
        break;
    }
    
    // Estrai numero dalla partita (formato "Partita X")
    int partitaNum = 0;
    int parsed = sscanf(game->gameName, "Partita %d", &partitaNum);
    
    // Verifica validità numero estratto
    if (parsed != 1 || partitaNum < 1 || partitaNum > 99) {
        // Fallback: cerca un nuovo numero disponibile
        partitaNum = findAvailableSaveNumber();
        
        if (partitaNum == 0) {
            clearScreen();
            printf("\n[!] ERRORE: Impossibile trovare un numero di salvataggio.\n");
            printf("\n    Premi un tasto per continuare...\n");
            getch_custom();
            break;
        }
        
        // Aggiorna il nome della partita con il numero valido
        snprintf(game->gameName, sizeof(game->gameName), "Partita %d", partitaNum);
    }
    
    // Crea percorso file
    char savePath[32];
    snprintf(savePath, sizeof(savePath), "save%d.txt", partitaNum);
    
    // Esegui salvataggio
    if (saveGame(&game->sudoku, savePath, game->cursorRow, game->cursorCol,
                game->errors, game->score, game->gameName)) {
        clearScreen();
        printf("\n[OK] Partita salvata con successo!\n");
        printf("     File: save%d.txt\n", partitaNum);
        printf("\n     Premi un tasto per continuare...\n");
        getch_custom();
    } else {
        clearScreen();
        printf("\n[!] ERRORE: Impossibile salvare la partita!\n");
        printf("    Verifica i permessi del file system.\n");
        printf("\n    Premi un tasto per continuare...\n");
        getch_custom();
    }
    break;
}

        case 'm': case 'M':
            game->gameState = STATE_MENU;
            break;

        default:
            break;
    }
}

/**
* Interfaccia per eliminare salvataggi
*/
void deleteSavedGameInterface() {
    int selected = 0;
    int numFiles = 0;
    char files[100][100];

    // Leggi i file "Salvataggio X.dat" presenti
    DIR* dir = opendir(".");
    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "Salvataggio ", 12) == 0 && strstr(entry->d_name, ".dat")) {
            strcpy(files[numFiles], entry->d_name);
            numFiles = numFiles + 1;
        }
    }
    closedir(dir);

    if (numFiles == 0) {
        printf("Nessun salvataggio trovato. Premi INVIO per tornare indietro.\n");
        while (getch_custom() != '\n');
        return;
    }

    // Ordina alfabeticamente
    int i = 0;
    while (i < numFiles - 1) {
        int j = i + 1;
        while (j < numFiles) {
            if (strcmp(files[i], files[j]) > 0) {
                char tmp[100];
                strcpy(tmp, files[i]);
                strcpy(files[i], files[j]);
                strcpy(files[j], tmp);
            }
            j = j + 1;
        }
        i = i + 1;
    }

    while (1) {
        clearScreen();

        // Cornice superiore
        i = 0;
        while (i < MARGIN_LEFT) {
            printf(" ");
            i = i + 1;
        }
        printf("+------------------------------------------------------------------------------+\n");
        printLineWithBordersCentered("ELIMINA SALVATAGGIO");
        i = 0;
        while (i < MARGIN_LEFT) {
            printf(" ");
            i = i + 1;
        }
        printf("+------------------------------------------------------------------------------+\n");

        // Elenco salvataggi
        i = 0;
        while (i < numFiles && i < 10) {
            int j = 0;
            while (j < MARGIN_LEFT) {
                printf(" ");
                j = j + 1;
            }
            if (i == selected)
                printf("| > %-74s |\n", files[i]);
            else
                printf("|   %-74s |\n", files[i]);
            i = i + 1;
        }

        // Spazio vuoto restante
        i = numFiles;
        while (i < 18) {
            int j = 0;
            while (j < MARGIN_LEFT) {
                printf(" ");
                j = j + 1;
            }
            printf("|                                                                              |\n");
            i = i + 1;
        }

        i = 0;
        while (i < MARGIN_LEFT) {
            printf(" ");
            i = i + 1;
        }
        printf("+------------------------------------------------------------------------------+\n");

        printf("\nUsa W/S per muoverti, INVIO per eliminare, ESC per uscire\n");

        int key = getch_custom();
        if (key == 'w' || key == 'W') {
            if (selected > 0) selected = selected - 1;
        } else if (key == 's' || key == 'S') {
            if (selected < numFiles - 1) selected = selected + 1;
        } else if (key == 27) { // ESC
            break;
        } else if (key == '\n') {
            clearScreen();
            i = 0;
            while (i < MARGIN_LEFT) {
                printf(" ");
                i = i + 1;
            }
            printf("Confermi di voler eliminare '%s'? (y/n): ", files[selected]);
            int confirm = getch_custom();
            if (confirm == 'y' || confirm == 'Y') {
                if (remove(files[selected]) == 0) {
                    i = selected;
                    while (i < numFiles - 1) {
                        strcpy(files[i], files[i + 1]);
                        i = i + 1;
                    }
                    numFiles = numFiles - 1;
                    if (selected >= numFiles && numFiles > 0) selected = selected - 1;
                } else {
                    printf("Errore nell'eliminazione.\n");
                    sleep(1);
                }
            }
        }
    }
}

