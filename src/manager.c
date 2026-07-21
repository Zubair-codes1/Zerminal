#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <raylib.h>
#include <poll.h>
#include <stdbool.h>
#include <termios.h>
#include <pthread.h>
#include "../include/screen.h"
#include "../include/font8x16.h"

void* pty_reader_thread(void* arg);

int main(void) {
    int master_fd = posix_openpt(O_RDWR);
    if (master_fd == -1) {
        printf("Error: PTY master not created.\n");
        return EXIT_FAILURE;
    }

    if (grantpt(master_fd) == -1) {
        printf("Error: PTY master could not own PTY slave.\n");
        return EXIT_FAILURE;
    }

    if (unlockpt(master_fd) == -1) {
        printf("Error: PTY slave was not unlocked.\n");
        return EXIT_FAILURE;
    }

    char* slave_path = ptsname(master_fd);
    if (slave_path == NULL) {
        printf("Error: Null slave path.\n");
        return EXIT_FAILURE;
    }

    pid_t childProcessID = fork();
    if (childProcessID < 0) {
        printf("Error: Failed to setup child process (fork failed).\n");
        return EXIT_FAILURE;
    } else if (childProcessID == 0) {
        if (setsid() == -1) {
            printf("Error: Session not created.\n");
            exit(EXIT_FAILURE);
        }

        int slave_fd = open(slave_path, O_RDWR);
        if (slave_fd == -1) {
            printf("Error: Slave file path couldnt be opened.\n");
            exit(EXIT_FAILURE);
        }

        if (ioctl(slave_fd, TIOCSCTTY, 0) == -1) {
            printf("Error: Could not assign slave to be the controlling terminal.\n");
            exit(EXIT_FAILURE);
        }

        int safe_stderr = dup(STDERR_FILENO);
        int std_fds[] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};

        for (int i = 0; i < 3; i++) {
            if (dup2(slave_fd, std_fds[i]) < 0) {
                perror("dup2 redirection failed");
                exit(1);
            }
        }

        close(slave_fd);

        char *shell_argv[] = {"./bin/shell", NULL};
        if (execvp("./bin/shell", shell_argv) == -1) {
            dprintf(safe_stderr, "Error: Zerminal execution failed.\n");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent Process Setup
        struct termios original_settings;
        tcgetattr(STDIN_FILENO, &original_settings);

        struct termios raw_settings = original_settings;
        raw_settings.c_lflag &= ~(ECHO | ICANON | ISIG);
        raw_settings.c_cc[VMIN] = 1;
        raw_settings.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw_settings);

        // Window & Terminal State Setup
        int initial_cols = 800 / FONT_WIDTH;
        int initial_rows = 600 / FONT_HEIGHT;
        initialise_screen(&terminal, initial_cols, initial_rows);

        // Bundle args for background thread
        ReaderThreadArgs thread_args = {
            .master_fd = master_fd,
            .term = &terminal,
            .parser = &global_parser
        };

        pthread_t reader_tid;
        if (pthread_create(&reader_tid, NULL, pty_reader_thread, &thread_args) != 0) {
            perror("Failed to create reader thread.");
            return EXIT_FAILURE;
        }
        pthread_detach(reader_tid);

        SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE);
        InitWindow(800, 600, "Raylib Terminal Emulator");
        SetTargetFPS(60);

        while (!WindowShouldClose()) {
            if (IsWindowResized()) {
                int newCols = GetScreenWidth() / FONT_WIDTH;
                int newRows = GetScreenHeight() / FONT_HEIGHT;

                pthread_mutex_lock(&canvas_mutex);
                resizeTerminal(&terminal, newCols, newRows);
                pthread_mutex_unlock(&canvas_mutex);

                // Notify shell of window dimension change
                struct winsize ws = {
                    .ws_row = newRows,
                    .ws_col = newCols,
                    .ws_xpixel = GetScreenWidth(),
                    .ws_ypixel = GetScreenHeight()
                };
                ioctl(master_fd, TIOCSWINSZ, &ws);
            }

            int key = GetCharPressed();
            while (key > 0) {
                if (key >= 32 && key <= 126) {
                    char c = (char)key;
                    write(master_fd, &c, 1);
                }
                key = GetCharPressed(); // Check if more characters are queued
            }

            if (IsKeyPressed(KEY_ESCAPE)) {
                char c = '\x1b'; // 0x1B - Escape key to exit Insert Mode in Vim
                write(master_fd, &c, 1);
            } else if (IsKeyPressed(KEY_UP)) {
                write(master_fd, "\x1b[A", 3);
            } else if (IsKeyPressed(KEY_DOWN)) {
                write(master_fd, "\x1b[B", 3);
            } else if (IsKeyPressed(KEY_RIGHT)) {
                write(master_fd, "\x1b[C", 3);
            } else if (IsKeyPressed(KEY_LEFT)) {
                write(master_fd, "\x1b[D", 3);
            }else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                char c = '\n';
                write(master_fd, &c, 1);
            } else if (IsKeyPressed(KEY_BACKSPACE)) {
                char c = 127; // ASCII DEL / Backspace
                write(master_fd, &c, 1);
            } else if (IsKeyPressed(KEY_TAB)) {
                char c = '\t';
                write(master_fd, &c, 1);
            }

            BeginDrawing();
            ClearBackground(BLACK);

            pthread_mutex_lock(&canvas_mutex);
            for (int r = 0; r < terminal.rows; r++) {
                for (int c = 0; c < terminal.cols; c++) {
                    Cell cell = terminal.grid[r * terminal.cols + c];
                    int x = c * FONT_WIDTH;
                    int y = r * FONT_HEIGHT;

                    // Draw background rectangle if filled
                    if (cell.bg_colour.a != 0) {
                        DrawRectangle(x, y, FONT_WIDTH, FONT_HEIGHT, cell.bg_colour);
                    }
                    
                    // Render bitmap character from font8x16 header
                    if (cell.character != ' ' && cell.character != 0) {
                        unsigned char ch = (unsigned char)cell.character;
                        for (int py = 0; py < 16; py++) {
                            // Grab byte row for character from font array
                            unsigned char row_byte = font8x16[ch][py];
                            for (int px = 0; px < 8; px++) {
                                // Check if pixel bit is active (MSB to LSB)
                                if (row_byte & (0x80 >> px)) {
                                    DrawPixel(x + px, y + py, cell.fg_colour);
                                }
                            }
                        }
                    }
                }
            }

            if (terminal.cursor.visible == true) {
                int cursor_x = terminal.cursor.x_pos * FONT_WIDTH;
                int cursor_y = terminal.cursor.y_pos * FONT_HEIGHT;

                DrawRectangle(cursor_x, cursor_y, FONT_WIDTH, FONT_HEIGHT, (Color){ 255, 255, 255, 160 });
            }

            pthread_mutex_unlock(&canvas_mutex);

            EndDrawing();
        }

        tcsetattr(STDIN_FILENO, TCSANOW, &original_settings);
        free(terminal.grid);
        CloseWindow();
        return 0;
    }
}

void* pty_reader_thread(void* arg) {
    ReaderThreadArgs *args = (ReaderThreadArgs *)arg;
    char buffer[1024];

    while (true) {
        int bytes_read = read(args->master_fd, buffer, sizeof(buffer));

        if (bytes_read <= 0) {
            printf("[SHELL EXITED via Threads]\n");
            break;
        }

        pthread_mutex_lock(&canvas_mutex);
        for (int i = 0; i < bytes_read; i++) {
            ProcessChar(args->term, args->parser, buffer[i]);
        }
        pthread_mutex_unlock(&canvas_mutex);
    }

    return NULL;
}
