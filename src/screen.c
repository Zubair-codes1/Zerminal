#include <stdio.h>
#include <pthread.h>
#include <stdint.h>

// each cell
typedef struct {
    char character;     // actual character being held
    uint32_t fg_colour; // foreground colour
    uint32_t bg_colour; // background colour
    uint8_t attrs;   // attributes
} Cell;


// Tracking Cursor
typedef struct {
    int x_pos;  // x pos
    int y_pos;  // y pos
} Cursor;


// cols and rows for grid
#define COLS 80
#define ROWS 25

// 2D array of cells
Cell pixels[ROWS][COLS];
