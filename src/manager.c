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
#include "../include/screen.h"
#include "../include/font8x16.h"

// function declarations
void* pty_reader_thread(void* arg);
void move_screen_down(void);

/**
 * Main function
 */
int main(void) {

    // creating a new PTY master
    int master_fd = posix_openpt(O_RDWR);

    if (master_fd == -1) {
        printf("Error: PTY master not created.\n");
        return EXIT_FAILURE;
    }

    // granting access to master to use PTY slave 
    int grant_success = grantpt(master_fd);

    if (grant_success == -1) {
        printf("Error: PTY master could not own PTY slave.\n");
        return EXIT_FAILURE;
    }

    // unlockding slave so it can be used by master
    int unlock_success = unlockpt(master_fd);

    if (unlock_success == -1) {
        printf("Error: PTY slave was not unlocked.\n");
        return EXIT_FAILURE;
    }

    // gets the string path to the slave
    char* slave_path = ptsname(master_fd);

    if (slave_path == NULL) {
        printf("Error: Null slave path.\n");
        return EXIT_FAILURE;
    }


    // child process - shell
    pid_t childProcessID = fork();

    if (childProcessID < 0) {
        printf("Error: Failed to setup child process (fork failed).\n");
        return EXIT_FAILURE;
    }else if (childProcessID == 0) {

        // create new session
        pid_t sessionID = setsid();

        if (sessionID == -1) {
            printf("Error: Session not created.\n");
            exit(EXIT_FAILURE);
        }

        // open slave path
        int slave_fd = open(slave_path, O_RDWR);

        if (slave_fd == -1) {
            printf("Error: Slave file path couldnt be opened.\n");
            exit(EXIT_FAILURE);
        }

        int terminal_success = ioctl(slave_fd, TIOCSCTTY, 0);

        if (terminal_success == -1) {
            printf("Error: Could not assign slave to be the controlling terminal.\n");
            exit(EXIT_FAILURE);
        }

        // saving STDERR_FILENO for error messages
        int safe_stderr = dup(STDERR_FILENO);

        // set input, output and errors to slave_fd
        int std_fds[] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};

        for (int i = 0; i < 3; i++) {
            if (dup2(slave_fd, std_fds[i]) < 0) {
                perror("dup2 redirection failed");
                exit(1); // Exit process if critical streams fail
            }
        }

        // closing the slave_fd
        int close_success = close(slave_fd);

        if (close_success == -1) {
            printf("Error: Could not close slave file descriptor.\n");
        }

        // executing the shell binary (zerminal)
        char *shell_argv[] = {"./bin/shell", NULL};

        int execution_success = execvp("./bin/shell", shell_argv);

        if (execution_success == -1) {
            dprintf(safe_stderr, "Error: Zerminal execution failed.\n");
            exit(EXIT_FAILURE);
        }


    }else {

        // original settings
        struct termios original_settings;

        tcgetattr(STDIN_FILENO, &original_settings);

        // raw settings
        struct termios raw_settings = original_settings;
        raw_settings.c_lflag &= ~(ECHO | ICANON | ISIG);

        raw_settings.c_cc[VMIN] = 1;    // reads data instantly (1 keystroke)
        raw_settings.c_cc[VTIME] = 0;   // does not wait

        tcsetattr(STDIN_FILENO, TCSANOW, &raw_settings);

        // array of structs for watching input and master_fd (polling)
        struct pollfd fds[2];
        int fds_size = 2;

        fds[0].fd = STDIN_FILENO;
        fds[0].events = POLLIN;     // any data available

        fds[1].fd = master_fd;
        fds[1].events = POLLIN;

        // initialise the screen
        initialise_screen();

        // creating thread to read master fd
        pthread_t reader_tid;

        if (pthread_create(&reader_tid, NULL, pty_reader_thread, &master_fd) != 0) {
            perror("Failed to create reader thread.");
            return EXIT_FAILURE;
        }

        pthread_detach(reader_tid);

        // screen constants
        const int font_width = 8;
        const int font_height = 16;
        const int scale = 2;

        const int screen_width = font_width * COLS * scale;
        const int screen_height = font_height * ROWS * scale;
        

        SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE);

        // initialise raylib window and set FPS
        InitWindow(screen_width, screen_height, "Zerminal");
        SetTargetFPS(60);

        // UI event loop
        while (!WindowShouldClose()) {

            // key pressed and write to master fd
            int keyPressed = GetCharPressed();
            while (keyPressed > 0) {
                char character = (char) keyPressed;
                write(master_fd, &character, 1);
                keyPressed = GetCharPressed();
            }

            // backspace and enter
            if (IsKeyPressed(KEY_ENTER)) {
                write(master_fd , "\r", 1);
            }

            if (IsKeyPressed(KEY_BACKSPACE)) {
                write(master_fd, "\x7f", 1);
            }

            // start drawing on screen
            BeginDrawing();
            ClearBackground(BLACK);

            pthread_mutex_lock(&canvas_mutex);

            // grid boxes (temporary)
            for (int row = 0; row < ROWS; row++) {
                for (int col = 0; col < COLS; col ++ ) {
                    int pixel_x = col * font_width * scale;
                    int pixel_y = row * font_height * scale;

                    unsigned char character = (unsigned char) pixels[row][col].character;

                    for (int y = 0; y < 16; y++) {
                        unsigned char byte_row = font8x16[character][y];

                        for (int x = 0; x < 8; x++) {

                            if (byte_row & (0x80 >> x)) {

                                int draw_x = pixel_x + (x * scale);
                                int draw_y = pixel_y + (y * scale);

                                DrawRectangle(draw_x, draw_y, scale, scale, WHITE);
                            }
                        }
                    }
                }
            }

            // cursor prompt location
            DrawRectangle(terminal_cursor.x_pos * font_width * scale, terminal_cursor.y_pos * font_height * scale, font_width * scale, font_height * scale, RAYWHITE);

            pthread_mutex_unlock(&canvas_mutex);

            EndDrawing();
        }
        
        CloseWindow();

        tcsetattr(STDIN_FILENO, TCSANOW, &original_settings);
    }

    return EXIT_SUCCESS;
}


/**
 * Function that runs a background thread
 * 
 * @param arg master fd
 */
void* pty_reader_thread(void* arg) {
    // get master fd from pointer
    int master_fd = *(int *)arg;

    char buffer[1024];

    // reading from master_fd
    while (true) {
        int bytes_read = read(master_fd, buffer, sizeof(buffer));

        if (bytes_read <= 0) {
            printf("[SHELL EXITED via Threads]");
            break;
        }

        pthread_mutex_lock(&canvas_mutex);

        // parsing each character that is returned
        for (int i = 0; i < bytes_read; i++) {
            char c = buffer[i];

            if (c == '\r') {
                terminal_cursor.x_pos = 0;

            } else if (c == '\n') {
                terminal_cursor.y_pos++;

            } else if (c == 127 || c == '\b') {
                if (terminal_cursor.x_pos > 0) {
                    terminal_cursor.x_pos--;
                    pixels[terminal_cursor.y_pos][terminal_cursor.x_pos].character = ' ';
                }
            } else {
                if (terminal_cursor.x_pos >= COLS) {
                    terminal_cursor.x_pos = 0;
                    terminal_cursor.y_pos += 1;
                }

                if (terminal_cursor.y_pos >= ROWS) {
                    move_screen_down();
                }

                pixels[terminal_cursor.y_pos][terminal_cursor.x_pos].character = c;
                terminal_cursor.x_pos++;

            } 

        }

        pthread_mutex_unlock(&canvas_mutex);
    }

    return NULL;
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

