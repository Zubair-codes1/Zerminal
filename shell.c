#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {

    while (true) {
        printf("-> ");

        char input[1024];

        fgets(input, 1024, stdin);
        input[strlen(input) - 1] = '\0';

        if (strlen(input) == 0) {
            continue;
        }

        char* token = strtok(input, " \t\n");
        if (strcmp(token, "cd") == 0) {
            char* fileDirectory = strtok(NULL, " \t\n");
            if (chdir(fileDirectory) == -1) {
                chdir(getenv("HOME"));
            }
        }else if (strcmp(token, "exit") == 0){
            exit(EXIT_SUCCESS);
        }else {
            int processID = fork();

            if (processID < 0) {
                printf("Shell: Fork failed\n");
                exit(EXIT_FAILURE);
            }else if (processID == 0) {
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

    }
    return EXIT_SUCCESS;
}
