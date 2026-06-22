#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>

/**
 * Main function
 */
int main(void) {

    // creating a new PTY master
    int master_fd = posix_openpt(O_RDWR);

    if (master_fd == -1) {
        printf("Error: PTY master not created.");
        return EXIT_FAILURE;
    }

    // granting access to master to use PTY slave 
    int grant_success = grantpt(master_fd);

    if (grant_success == -1) {
        printf("Error: PTY master could not own PTY slave");
        return EXIT_FAILURE;
    }

    // unlockding slave so it can be used by master
    int unlock_success = unlockpt(master_fd);

    if (unlock_success == -1) {
        printf("Error: PTY slave was not unlocked.");
        return EXIT_FAILURE;
    }

    // gets the string path to the slave
    char* slave_path = ptsname(master_fd);

    if (slave_path == NULL) {
        printf("Error: Null slave path.");
        return EXIT_FAILURE;
    }


    // child process - shell
    pid_t childProcessID = fork();

    if (childProcessID < 0) {
        printf("Error: Failed to setup child process (fork failed).");
        return EXIT_FAILURE;
    }else if (childProcessID == 0) {

        // create new session
        pid_t sessionID = setsid();

        if (sessionID == -1) {
            printf("Error: Session not created.");
            exit(EXIT_FAILURE);
        }

        // open slave path
        int slave_fd = open(slave_path, O_RDWR);

        if (slave_fd == -1) {
            printf("Error: Slave file path couldnt be opened.");
            exit(EXIT_FAILURE);
        }

        int terminal_success = ioctl(slave_fd, TIOCSCTTY, 0);

        if (terminal_success == -1) {
            printf("Error: Could not assign slave to be the controlling terminal.");
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
            printf("Error: Could not close slave file descriptor.");
        }

        // executing the shell binary (zerminal)
        char *shell_argv[] = {"shell", NULL};

        int execution_success = execvp("../bin/zerminal", shell_argv);

        if (execution_success == -1) {
            dprintf(safe_stderr, "Error: Zerminal execution failed.");
            exit(EXIT_FAILURE);
        }


    }else {

        struct pollfd fds[2];
    }

    return EXIT_SUCCESS;
}
