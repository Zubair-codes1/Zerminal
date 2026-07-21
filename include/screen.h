#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>
#include <pthread.h>
#include <raylib.h>

#define DEFAULT_COLS 80
#define DEFAULT_ROWS 25

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

// Display cell
typedef struct {
    char character;
    Color fg_colour;
    Color bg_colour;
    uint8_t attrs;
} Cell;

// Tracking Cursor
typedef struct {
    int x_pos;
    int y_pos;
    bool visible; // <-- Add this
} Cursor;

// Terminal buffer
typedef struct {
    Cell *grid;     // Dynamic 1D grid array (size = rows * cols)
    int cols;
    int rows;
    Cursor cursor;
} Terminal;

// ANSI ESC Parser state
typedef enum {
    STATE_NORMAL,
    STATE_ESC,
    STATE_CSI
} ParseState;

typedef struct {
    ParseState state;
    char paramBuffer[32];
    int paramIdx;
    bool isPrivate;
    Color currentFg;
    Color currentBg;
} ANSIParser;

// Thread args bundle for reader thread
typedef struct {
    int master_fd;
    Terminal *term;
    ANSIParser *parser;
} ReaderThreadArgs;

// Global declarations
extern Terminal terminal;
extern ANSIParser global_parser;
extern pthread_mutex_t canvas_mutex;

// Function declarations
void initialise_screen(Terminal *term, int cols, int rows);
void resizeTerminal(Terminal *term, int newCols, int newRows);
void move_screen_down(Terminal *term);
void ProcessChar(Terminal *term, ANSIParser *parser, char c);
void ExecuteCSICommand(Terminal *term, ANSIParser *parser, char cmd);

#endif
