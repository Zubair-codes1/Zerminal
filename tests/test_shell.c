#include "test.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#ifndef SHELL_PATH
#define SHELL_PATH "./bin/shell"
#endif

/**
 * Spawns a fresh shell process, writes `input` to its stdin, and reads
 * everything it prints back until the process exits (or a timeout hits).
 *
 * Returns true if the process was reaped within the timeout.
 */
bool run_shell(const char* input, char* output, size_t output_size, int timeout_seconds) {
    int in_pipe[2];   // parent writes -> child stdin
    int out_pipe[2];  // child stdout -> parent reads

    if (pipe(in_pipe) == -1 || pipe(out_pipe) == -1) {
        return false;
    }

    pid_t pid = fork();

    if (pid < 0) {
        return false;
    } else if (pid == 0) {
        // child: wire up stdin/stdout to the pipes
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);
        dup2(out_pipe[1], STDERR_FILENO);

        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);

        execl(SHELL_PATH, SHELL_PATH, (char*)NULL);
        _exit(127); // execl failed
    }

    // parent
    close(in_pipe[0]);
    close(out_pipe[1]);

    write(in_pipe[1], input, strlen(input));
    close(in_pipe[1]); // signal EOF once we're done sending commands

    size_t total_read = 0;
    time_t start = time(NULL);

    while (total_read < output_size - 1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(out_pipe[0], &fds);

        struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
        int ready = select(out_pipe[0] + 1, &fds, NULL, NULL, &tv);

        if (ready > 0) {
            ssize_t n = read(out_pipe[0], output + total_read, output_size - 1 - total_read);
            if (n <= 0) break; // child closed its stdout (likely exited)
            total_read += (size_t)n;
        }

        if (difftime(time(NULL), start) > timeout_seconds) break;
    }
    output[total_read] = '\0';
    close(out_pipe[0]);

    // give the child a brief grace period to actually become reapable
    // after closing its stdout, then fall back to killing it if it's
    // truly stuck (e.g. shell.c's EOF-handling loop never calling exit)
    int status;
    pid_t waited = 0;
    for (int attempt = 0; attempt < 20 && waited == 0; attempt++) {
        waited = waitpid(pid, &status, WNOHANG);
        if (waited == 0) usleep(50000); // 50ms
    }

    if (waited == 0) {
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
        return false;
    }

    return true;
}

TEST(test_simple_command_runs) {
    char output[4096];
    bool reaped = run_shell("echo hello\nexit\n", output, sizeof(output), 5);

    ASSERT_TRUE(reaped);
    ASSERT_STR_CONTAINS(output, "hello");
}

TEST(test_cd_changes_directory) {
    char output[4096];
    bool reaped = run_shell("cd /tmp\npwd\nexit\n", output, sizeof(output), 5);

    ASSERT_TRUE(reaped);
    ASSERT_STR_CONTAINS(output, "/tmp");
}

TEST(test_cd_no_args_goes_home) {
    char output[4096];
    char script[256];
    const char* home = getenv("HOME");
    snprintf(script, sizeof(script), "cd /tmp\ncd\npwd\nexit\n");

    bool reaped = run_shell(script, output, sizeof(output), 5);

    ASSERT_TRUE(reaped);
    if (home != NULL) {
        ASSERT_STR_CONTAINS(output, home);
    }
}

TEST(test_cd_invalid_directory_reports_error) {
    char output[4096];
    bool reaped = run_shell("cd /this/does/not/exist\nexit\n", output, sizeof(output), 5);

    ASSERT_TRUE(reaped);
    ASSERT_STR_CONTAINS(output, "No such directory");
}

TEST(test_unknown_command_reports_error) {
    char output[4096];
    bool reaped = run_shell("thiscommanddoesnotexist123\nexit\n", output, sizeof(output), 5);

    ASSERT_TRUE(reaped);
    ASSERT_STR_CONTAINS(output, "No such command");
}

TEST(test_pipe_between_two_commands) {
    char output[4096];
    bool reaped = run_shell("printf 'a\\nb\\nc\\n' | wc -l\nexit\n", output, sizeof(output), 5);

    ASSERT_TRUE(reaped);
    ASSERT_STR_CONTAINS(output, "3");
}

TEST(test_output_redirection_creates_file) {
    char output[4096];
    char script[256];
    snprintf(script, sizeof(script),
        "echo redirected_text > /tmp/zerminal_test_out.txt\ncat /tmp/zerminal_test_out.txt\nexit\n");

    bool reaped = run_shell(script, output, sizeof(output), 5);

    ASSERT_TRUE(reaped);
    ASSERT_STR_CONTAINS(output, "redirected_text");

    remove("/tmp/zerminal_test_out.txt");
}

TEST(test_output_redirection_truncates_existing_file) {
    // pre-populate the file with something longer than what we redirect next
    FILE* f = fopen("/tmp/zerminal_test_trunc.txt", "w");
    fprintf(f, "this line should be gone after truncation\n");
    fclose(f);

    char output[4096];
    char script[256];
    snprintf(script, sizeof(script),
        "echo short > /tmp/zerminal_test_trunc.txt\ncat /tmp/zerminal_test_trunc.txt\nexit\n");

    bool reaped = run_shell(script, output, sizeof(output), 5);

    ASSERT_TRUE(reaped);
    ASSERT_STR_CONTAINS(output, "short");
    ASSERT_TRUE(strstr(output, "should be gone") == NULL);

    remove("/tmp/zerminal_test_trunc.txt");
}

TEST(test_append_redirection_appends_not_overwrites) {
    remove("/tmp/zerminal_test_append.txt");

    char output[4096];
    char script[512];
    snprintf(script, sizeof(script),
        "echo first_line >> /tmp/zerminal_test_append.txt\n"
        "echo second_line >> /tmp/zerminal_test_append.txt\n"
        "cat /tmp/zerminal_test_append.txt\nexit\n");

    bool reaped = run_shell(script, output, sizeof(output), 5);

    ASSERT_TRUE(reaped);
    ASSERT_STR_CONTAINS(output, "first_line");
    ASSERT_STR_CONTAINS(output, "second_line");

    remove("/tmp/zerminal_test_append.txt");
}

TEST(test_input_redirection_reads_from_file) {
    FILE* f = fopen("/tmp/zerminal_test_in.txt", "w");
    fprintf(f, "content_from_file\n");
    fclose(f);

    char output[4096];
    bool reaped = run_shell("cat < /tmp/zerminal_test_in.txt\nexit\n", output, sizeof(output), 5);

    ASSERT_TRUE(reaped);
    ASSERT_STR_CONTAINS(output, "content_from_file");

    remove("/tmp/zerminal_test_in.txt");
}

TEST(test_exit_terminates_the_shell) {
    char output[4096];
    bool reaped = run_shell("exit\n", output, sizeof(output), 5);
    ASSERT_TRUE(reaped);
}

/**
 * run all tests
 */
int main(void) {
    test_simple_command_runs();
    test_cd_changes_directory();
    test_cd_no_args_goes_home();
    test_cd_invalid_directory_reports_error();
    test_unknown_command_reports_error();
    test_pipe_between_two_commands();
    test_output_redirection_creates_file();
    test_output_redirection_truncates_existing_file();
    test_append_redirection_appends_not_overwrites();
    test_input_redirection_reads_from_file();
    test_exit_terminates_the_shell();

    TEST_SUMMARY();
    return tests_failed != 0;
}