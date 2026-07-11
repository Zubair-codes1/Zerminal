#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>
#include <pthread.h>

// cols and rows for grid
#define COLS 80
#define ROWS 25

// each cell
typedef struct {
    char character;     // actual character being held
    uint32_t fg_colour; // foreground colour
    uint32_t bg_colour; // background colour
    uint8_t attrs;      // attributes
} Cell;

// Tracking Cursor
typedef struct {
    int x_pos;  // x pos
    int y_pos;  // y pos
} Cursor;

// Extern declarations: These variables exist globally
extern Cell pixels[ROWS][COLS];
extern Cursor terminal_cursor;

// canvas mutex
extern pthread_mutex_t canvas_mutex;

// function to initialise terminal screen
void initialise_screen(void);

// moving screen down
void move_screen_down(void);

#endif