#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/screen.h"

// Terminal instance & parser state
Terminal terminal = {0};
ANSIParser global_parser = { .currentFg = RAYWHITE, .currentBg = BLACK };

// Canvas mutex
pthread_mutex_t canvas_mutex = PTHREAD_MUTEX_INITIALIZER;

void initialise_screen(Terminal *term, int cols, int rows) {
    term->cols = cols;
    term->rows = rows;
    term->cursor.x_pos = 0;
    term->cursor.y_pos = 0;
    term->cursor.visible = true;

    term->grid = calloc(cols * rows, sizeof(Cell));

    // Clear initial screen cells
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int index = r * cols + c;
            term->grid[index].character = ' ';
            term->grid[index].bg_colour = BLACK;
            term->grid[index].fg_colour = RAYWHITE;
            term->grid[index].attrs = 0;
        }
    }
}

/**
 * Dynamically resizes the terminal grid and reflows cells
 */
void resizeTerminal(Terminal *term, int newCols, int newRows) {
    if (newCols <= 0 || newRows <= 0) return;

    Cell *newGrid = calloc(newCols * newRows, sizeof(Cell));
    
    // Fill new grid with default spaces
    for (int i = 0; i < newCols * newRows; i++) {
        newGrid[i].character = ' ';
        newGrid[i].bg_colour = BLACK;
        newGrid[i].fg_colour = RAYWHITE;
    }

    // Copy existing text over where it fits
    int minRows = term->rows < newRows ? term->rows : newRows;
    int minCols = term->cols < newCols ? term->cols : newCols;
    
    for (int r = 0; r < minRows; r++) {
        for (int c = 0; c < minCols; c++) {
            newGrid[r * newCols + c] = term->grid[r * term->cols + c];
        }
    }
    
    free(term->grid);
    term->grid = newGrid;
    term->cols = newCols;
    term->rows = newRows;

    // Clamp cursor within new boundaries
    if (term->cursor.x_pos >= term->cols) term->cursor.x_pos = term->cols - 1;
    if (term->cursor.y_pos >= term->rows) term->cursor.y_pos = term->rows - 1;
}

/**
 * Scrolls the buffer up by 1 row when bottom line is reached
 */
void move_screen_down(Terminal *term) {
    for (int row = 1; row < term->rows; row++) {
        for (int col = 0; col < term->cols; col++) {
            term->grid[(row - 1) * term->cols + col] = term->grid[row * term->cols + col];
        }
    }

    // Clear bottom row
    for (int col = 0; col < term->cols; col++) {
        int index = (term->rows - 1) * term->cols + col;
        term->grid[index].character = ' ';
        term->grid[index].fg_colour = RAYWHITE;
        term->grid[index].bg_colour = BLACK;
    }

    term->cursor.y_pos = term->rows - 1;
}

void ProcessChar(Terminal *term, ANSIParser *parser, char c) {
    switch (parser->state) {
        case STATE_NORMAL:
            if (c == '\x1b') {
                parser->state = STATE_ESC;
            } else if (c == '\n') {
                term->cursor.x_pos = 0;
                term->cursor.y_pos++;
                if (term->cursor.y_pos >= term->rows) {
                    move_screen_down(term);
                }
            } else if (c == '\r') {
                term->cursor.x_pos = 0;
            } else if (c == 127 || c == '\b') {
                if (term->cursor.x_pos > 0) {
                    term->cursor.x_pos--;
                    int index = term->cursor.y_pos * term->cols + term->cursor.x_pos;
                    term->grid[index].character = ' ';
                }
            } else {
                if (term->cursor.x_pos >= term->cols) {
                    term->cursor.x_pos = 0;
                    term->cursor.y_pos++;
                }

                if (term->cursor.y_pos >= term->rows) {
                    move_screen_down(term);
                }

                int index = term->cursor.y_pos * term->cols + term->cursor.x_pos;
                term->grid[index].character = c;
                term->grid[index].fg_colour = parser->currentFg;
                term->grid[index].bg_colour = parser->currentBg;
                
                term->cursor.x_pos++;
            }
            break;

        case STATE_ESC:
            if (c == '[') {
                parser->state = STATE_CSI;
                parser->paramIdx = 0;
                parser->paramBuffer[0] = '\0';
                parser->isPrivate = false; // Reset private mode flag
            } else {
                parser->state = STATE_NORMAL;
            }
            break;

        case STATE_CSI:
            if (c == '?') {
                parser->isPrivate = true; // Mark as private sequence like ?25h
            } else if ((c >= '0' && c <= '9') || c == ';') {
                if (parser->paramIdx < (int)sizeof(parser->paramBuffer) - 1) {
                    parser->paramBuffer[parser->paramIdx++] = c;
                    parser->paramBuffer[parser->paramIdx] = '\0';
                }
            } else {
                // Command terminator reached ('h', 'l', 'm', 'J', etc.)
                ExecuteCSICommand(term, parser, c);
                parser->state = STATE_NORMAL;
            }
            break;
    }
}

void ExecuteCSICommand(Terminal *term, ANSIParser *parser, char cmd) {
    int code = atoi(parser->paramBuffer);

    // 1. Handle DEC Private Mode sequences (e.g., ESC [ ? 25 h / l)
    if (parser->isPrivate) {
        if (code == 25) {
            if (cmd == 'h') {
                term->cursor.visible = true;
            } else if (cmd == 'l') {
                term -> cursor.visible = false;
            }
        }
        return; // Consume private sequences so they don't print!
    }

    if (cmd == 'm') { // SGR color execution
        int code = atoi(parser->paramBuffer);
        switch (code) {
            case 0:  
                parser->currentFg = RAYWHITE;
                parser->currentBg = BLACK;
                break;
            case 30: parser->currentFg = BLACK; break;
            case 31: parser->currentFg = RED; break;
            case 32: parser->currentFg = GREEN; break;
            case 33: parser->currentFg = YELLOW; break;
            case 34: parser->currentFg = BLUE; break;
            case 35: parser->currentFg = MAGENTA; break;
            case 36: parser->currentFg = SKYBLUE; break;
            case 37: parser->currentFg = RAYWHITE; break;
        }
    } else if (cmd == 'J') { // Clear screen
        if (atoi(parser->paramBuffer) == 2) {
            for (int i = 0; i < term->cols * term->rows; i++) {
                term->grid[i].character = ' ';
                term->grid[i].bg_colour = BLACK;
                term->grid[i].fg_colour = RAYWHITE;
            }
            term->cursor.x_pos = 0;
            term->cursor.y_pos = 0;
        }
    }else if (cmd == 'H' || cmd == 'f') {
        int row = 1;
        int col = 1;

        // Parse "row;col" from paramBuffer (e.g., "24;1")
        if (strlen(parser->paramBuffer) > 0) {
            sscanf(parser->paramBuffer, "%d;%d", &row, &col);
        }

        // ANSI sequences are 1-indexed, convert to 0-indexed terminal grid
        term->cursor.x_pos = (col > 0) ? col - 1 : 0;
        term->cursor.y_pos = (row > 0) ? row - 1 : 0;

        // Clamp inside bounds
        if (term->cursor.x_pos >= term->cols) term->cursor.x_pos = term->cols - 1;
        if (term->cursor.y_pos >= term->rows) term->cursor.y_pos = term->rows - 1;
    }else if (cmd == 'K') {
        int mode = atoi(parser->paramBuffer); // Default 0
        int start_col = term->cursor.x_pos;
        int end_col = term->cols;

        if (mode == 0) { 
            // Clear from cursor to end of line
            for (int c = start_col; c < end_col; c++) {
                int index = term->cursor.y_pos * term->cols + c;
                term->grid[index].character = ' ';
                term->grid[index].bg_colour = BLACK;
                term->grid[index].fg_colour = RAYWHITE;
            }
        } else if (mode == 2) {
            // Clear entire line
            for (int c = 0; c < term->cols; c++) {
                int index = term->cursor.y_pos * term->cols + c;
                term->grid[index].character = ' ';
                term->grid[index].bg_colour = BLACK;
                term->grid[index].fg_colour = RAYWHITE;
            }
        }
    }
}
