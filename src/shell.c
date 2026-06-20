#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

// function declarations
void handleInput(void);
void handlePipes(char* input, char* pipes_pos);
void handleOutputRedirection(char* input, char* redirect_pos);
void handleInputRedirection(char* input, char* redirect_pos);
void runStandardCommands(char* input);
void runInBuiltCommands(char* token);

/**
 * Runs the main loop of the shell
 */
int main(void) {

    signal(SIGINT, SIG_IGN);
    char directory[1024];

    while (true) {
        if (getcwd(directory, sizeof(directory)) == NULL) {
            printf("Shell: Current directory doesnt exist.\n");
            break;
        }
        printf("\n%s -> ", directory);

        handleInput();
        
    }
    return EXIT_SUCCESS;
}

/**
 * Hnadles getting input and checking for certain things within the input
 * 
 * Calls the functions based on the input
 */
void handleInput(void) {
    char input[1024];

    // check for null entries / signals
    if (fgets(input, 1024, stdin) == NULL) {
        printf("\n");
        return;
    }

    input[strlen(input) - 1] = '\0';

    printf("\n");

    if (strlen(input) == 0) {
        return;
    }

    char* pipe_pos = strchr(input, '|');
    char* output_redirect_pos = strchr(input, '>');
    char* input_redirect_pos = strchr(input, '<');

    if (pipe_pos != NULL) {
        handlePipes(input, pipe_pos);
    }else if (output_redirect_pos != NULL) {
        handleOutputRedirection(input, output_redirect_pos);
    }else if (input_redirect_pos != NULL) {
        handleInputRedirection(input, input_redirect_pos);
    }else {
        runStandardCommands(input);
    }
}

/**
 * Handles pipes between two commands ( | )
 * 
 * @param input full input string
 * @param pipe_pos location of pipe in the string 
 */
void handlePipes(char* input, char* pipe_pos) {
    *pipe_pos = '\0';          // left command is now input
    char *right = pipe_pos + 1; // right command starts here
    char *left = input;

    int fds[2];
    pipe(fds);

    // sub process 1 - handles output
    int processID1 = fork();

    if (processID1 < 0) {
        printf("Shell: Fork Failed.\n");
        exit(EXIT_FAILURE);
    }else if (processID1 == 0) {

        signal(SIGINT, SIG_DFL);
        dup2(fds[1], STDOUT_FILENO);
        close(fds[0]);
        close(fds[1]);

        left = strtok(left, " \t\n");

        char *argv[64];
        int argc = 0;

        while (left != NULL && argc < 63) {
            argv[argc++] = left;
            left = strtok(NULL, " \t\n");
        }

        argv[argc] = NULL;  // null for execvp

        char* command = argv[0];

        if (execvp(command, argv) == -1) {
            printf("Shell: No such command\n");
            exit(EXIT_FAILURE);
        }

    }
    
    // sub process 2 - handles input

    int processID2 = fork();
    if (processID2 < 0) {
        printf("Shell: Fork Failed.\n");
        exit(EXIT_FAILURE);
    }else if (processID2 == 0) {

        signal(SIGINT, SIG_DFL);
        dup2(fds[0], STDIN_FILENO);
        close(fds[0]);
        close(fds[1]);

        right = strtok(right, " \t\n");

        char *argv[64];
        int argc = 0;

        while (right != NULL && argc < 63) {
            argv[argc++] = right;
            right = strtok(NULL, " \t\n");
        }

        argv[argc] = NULL;  // null for execvp

        char* command = argv[0];

        if (execvp(command, argv) == -1) {
            printf("Shell: No such command\n");
            exit(EXIT_FAILURE);
        }

    }

    // parent process

    // closing file descriptors
    close(fds[0]);
    close(fds[1]);

    // waiting for sub processes to finish
    waitpid(processID1, NULL, 0);
    waitpid(processID2, NULL, 0);
}

/**
 * Handle output redirections ( > )
 * 
 * @param input all input
 * @param redirect_pos redirection symbol position (<)
 */
void handleOutputRedirection(char* input, char* redirect_pos) {
    *redirect_pos = '\0';          // left command is now input
    char *right = redirect_pos + 1; // right command starts here
    char *left = input;

    int processID = fork();

    if (processID < 0) {
        printf("Shell: Fork failed.\n");
        exit(EXIT_FAILURE);
    }else if (processID == 0) {
        signal(SIGINT, SIG_DFL);
        right = strtok(right, " \t\n");
        int fd = open(right, O_CREAT | O_TRUNC | O_WRONLY, 0644);
        dup2(fd, STDOUT_FILENO);
        close(fd);

        left = strtok(left, " \t\n");

        char *argv[64];
        int argc = 0;

        while (left != NULL && argc < 63) {
            argv[argc++] = left;
            left = strtok(NULL, " \t\n");
        }

        argv[argc] = NULL;  // null for execvp

        char* command = argv[0];

        if (execvp(command, argv) == -1) {
            printf("Shell: No such command\n");
            exit(EXIT_FAILURE);
        }

    }else {
        waitpid(processID, NULL, 0);
    }
}

/**
 * handle input redirection ( < )
 * 
 * @param input full input line
 * @param redirect_pos position of <
 */
void handleInputRedirection(char* input, char* redirect_pos) {
    *redirect_pos = '\0';          // left command is now input
    char *right = redirect_pos + 1; // right command starts here
    char *left = input;

    int processID = fork();

    if (processID < 0) {
        printf("Shell: Fork failed.\n");
        exit(EXIT_FAILURE);
    }else if (processID == 0) {
        signal(SIGINT, SIG_DFL);
        right = strtok(right, " \t\n");
        int fd = open(right, O_RDONLY);
        dup2(fd, STDIN_FILENO);
        close(fd);

        left = strtok(left, " \t\n");

        char *argv[64];
        int argc = 0;

        while (left != NULL && argc < 63) {
            argv[argc++] = left;
            left = strtok(NULL, " \t\n");
        }

        argv[argc] = NULL;  // null for execvp

        char* command = argv[0];

        if (execvp(command, argv) == -1) {
            printf("Shell: No such command\n");
            exit(EXIT_FAILURE);
        }

    }else {
        waitpid(processID, NULL, 0);
    }
}

/**
 * Function that runs all inbuilt and external commands
 * 
 * @param input input string
 */
void runStandardCommands(char* input) {
    char* token = strtok(input, " \t\n");
    if (strcmp(token, "cd") == 0) {
        char* fileDirectory = strtok(NULL, " \t\n");
        if (fileDirectory == NULL) {
            chdir(getenv("HOME"));
        }else {
            if (chdir(fileDirectory) == -1) {
                printf("Shell: No such directory or file exists.\n");
                return;
            }
        }
    }else if (strcmp(token, "exit") == 0){
        exit(EXIT_SUCCESS);
    }else {
        runInBuiltCommands(token);
    }
}

/**
 * Function that runs all the inbuilt commands
 * 
 * @param token tokens/input
 */
void runInBuiltCommands(char* token) {

    int processID = fork();

    if (processID < 0) {
        printf("Shell: Fork failed\n");
        exit(EXIT_FAILURE);
    }else if (processID == 0) {
        signal(SIGINT, SIG_DFL);
        char *argv[64];
        int argc = 0;

        while (token != NULL && argc < 63) {
            argv[argc++] = token;
            token = strtok(NULL, " \t\n");
        }

        argv[argc] = NULL;  // null for execvp

        char* command = argv[0];

        if (execvp(command, argv) == -1) {
            printf("Shell: No such command\n");
            exit(EXIT_FAILURE);
        }

    }else {
        waitpid(processID, NULL, 0);
    }
}
