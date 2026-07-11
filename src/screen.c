#include <stdio.h>
#include "../include/screen.h"

// cells/pixels
Cell pixels[ROWS][COLS];
// terminal cursor
Cursor terminal_cursor;

// canvas mutex
pthread_mutex_t canvas_mutex = PTHREAD_MUTEX_INITIALIZER;

void initialise_screen(void) {

    // intitial cursor positions
    terminal_cursor.x_pos = 0;
    terminal_cursor.y_pos = 0;

    // clearing screen cells
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            pixels[row][col].character = ' ';
            pixels[row][col].bg_colour = 0x000000FF; // black bg_colour
            pixels[row][col].fg_colour = 0xFFFFFFFF; // white fg_colour
            pixels[row][col].attrs = 0;     // no attributes
        }
    }
}

/**
 * Moves the screen down after max line is reached
 */
void move_screen_down(void) {
    for (int row = 1; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            pixels[row-1][col] = pixels[row][col];
        }
    }

    for (int col = 0; col < COLS; col++) {
        pixels[ROWS - 1][col].character = ' ';
        pixels[ROWS - 1][col].fg_colour = 0xFFFFFFFF;
        pixels[ROWS - 1][col].bg_colour = 0x000000FF;
    }

    terminal_cursor.y_pos = ROWS - 1;
}
