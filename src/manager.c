#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <termios.h>

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

        // polling loop
        while (true) {

            // polls constantly for an activity
            int activity = poll(fds, fds_size, -1);

            if (activity == -1) {
                perror("Poll error");
                break;
            }

            // User input is read
            if (fds[0].revents & POLLIN) {
                char buffer[1024];
                int bytes_read = read(fds[0].fd, buffer, sizeof(buffer));

                write(master_fd, buffer, bytes_read);
            }

            // master_fd is read
            if (fds[1].revents & POLLIN) {
                char buffer[1024];
                int bytes_read = read(fds[1].fd, buffer, sizeof(buffer));

                if (bytes_read <= 0) {
                    printf("[SHELL EXITED]\n");
                    break;
                }

                write(STDOUT_FILENO, buffer, bytes_read);
            }
        }

        tcsetattr(STDIN_FILENO, TCSANOW, &original_settings);
    }

    return EXIT_SUCCESS;
}
