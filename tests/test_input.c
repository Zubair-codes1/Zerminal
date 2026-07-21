#define _POSIX_C_SOURCE 200809L

#include "test.h"
#include "screen.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

/* --- Simulated Key Input & Filtering Logic --- */

// Simulates sending printable characters filtered by the ASCII range check
static void process_char_input(int master_fd, int key) {
    if (key >= 32 && key <= 126) {
        char c = (char)key;
        write(master_fd, &c, 1);
    }
}

// Simulates key mapping for special/navigation control keys
static void process_special_key_input(int master_fd, int key_code) {
    // Standard keycodes matching Raylib constants
    enum {
        KEY_ENTER_CODE = 257,
        KEY_KP_ENTER_CODE = 335,
        KEY_BACKSPACE_CODE = 259,
        KEY_TAB_CODE = 258,
        KEY_ESCAPE_CODE = 256,
        KEY_UP_CODE = 265,
        KEY_DOWN_CODE = 264,
        KEY_RIGHT_CODE = 262,
        KEY_LEFT_CODE = 263
    };

    if (key_code == KEY_ENTER_CODE || key_code == KEY_KP_ENTER_CODE) {
        char c = '\n';
        write(master_fd, &c, 1);
    } else if (key_code == KEY_BACKSPACE_CODE) {
        char c = 127;
        write(master_fd, &c, 1);
    } else if (key_code == KEY_TAB_CODE) {
        char c = '\t';
        write(master_fd, &c, 1);
    } else if (key_code == KEY_ESCAPE_CODE) {
        char c = '\x1b';
        write(master_fd, &c, 1);
    } else if (key_code == KEY_UP_CODE) {
        write(master_fd, "\x1b[A", 3);
    } else if (key_code == KEY_DOWN_CODE) {
        write(master_fd, "\x1b[B", 3);
    } else if (key_code == KEY_RIGHT_CODE) {
        write(master_fd, "\x1b[C", 3);
    } else if (key_code == KEY_LEFT_CODE) {
        write(master_fd, "\x1b[D", 3);
    }
}

/* --- Tests --- */

TEST(test_printable_char_filtering_suppresses_control_codes) {
    int pipe_fds[2];
    ASSERT_TRUE(pipe(pipe_fds) == 0);

    int write_fd = pipe_fds[1];
    int read_fd = pipe_fds[0];

    // Simulate input stream containing non-printable codes, dots, and letters
    int test_keys[] = { 0x1B, 27, 12, 'a', '.', 'Z', 127, 200 };
    int count = sizeof(test_keys) / sizeof(test_keys[0]);

    for (int i = 0; i < count; i++) {
        process_char_input(write_fd, test_keys[i]);
    }

    char buf[16] = {0};
    ssize_t bytes_read = read(read_fd, buf, sizeof(buf) - 1);

    // Only 'a', '.', and 'Z' (ASCII 32..126) should pass through
    ASSERT_EQ(bytes_read, 3);
    ASSERT_STR_EQ(buf, "a.Z");

    close(write_fd);
    close(read_fd);
}

TEST(test_arrow_left_sends_ansi_sequence) {
    int pipe_fds[2];
    ASSERT_TRUE(pipe(pipe_fds) == 0);

    process_special_key_input(pipe_fds[1], 263); // KEY_LEFT

    char buf[10] = {0};
    ssize_t n = read(pipe_fds[0], buf, sizeof(buf) - 1);

    ASSERT_EQ(n, 3);
    ASSERT_STR_EQ(buf, "\x1b[D");

    close(pipe_fds[0]);
    close(pipe_fds[1]);
}

TEST(test_arrow_right_sends_ansi_sequence) {
    int pipe_fds[2];
    ASSERT_TRUE(pipe(pipe_fds) == 0);

    process_special_key_input(pipe_fds[1], 262); // KEY_RIGHT

    char buf[10] = {0};
    ssize_t n = read(pipe_fds[0], buf, sizeof(buf) - 1);

    ASSERT_EQ(n, 3);
    ASSERT_STR_EQ(buf, "\x1b[C");

    close(pipe_fds[0]);
    close(pipe_fds[1]);
}

TEST(test_arrow_up_and_down_send_ansi_sequences) {
    int pipe_fds[2];
    ASSERT_TRUE(pipe(pipe_fds) == 0);

    process_special_key_input(pipe_fds[1], 265); // KEY_UP
    process_special_key_input(pipe_fds[1], 264); // KEY_DOWN

    char buf[10] = {0};
    ssize_t n = read(pipe_fds[0], buf, sizeof(buf) - 1);

    ASSERT_EQ(n, 6);
    ASSERT_STR_EQ(buf, "\x1b[A\x1b[B");

    close(pipe_fds[0]);
    close(pipe_fds[1]);
}

TEST(test_escape_and_control_keys) {
    int pipe_fds[2];
    ASSERT_TRUE(pipe(pipe_fds) == 0);

    process_special_key_input(pipe_fds[1], 256); // ESCAPE
    process_special_key_input(pipe_fds[1], 259); // BACKSPACE
    process_special_key_input(pipe_fds[1], 258); // TAB

    char buf[10] = {0};
    ssize_t n = read(pipe_fds[0], buf, sizeof(buf) - 1);

    ASSERT_EQ(n, 3);
    ASSERT_EQ(buf[0], '\x1b');
    ASSERT_EQ(buf[1], 127);
    ASSERT_EQ(buf[2], '\t');

    close(pipe_fds[0]);
    close(pipe_fds[1]);
}

int main(void) {
    test_printable_char_filtering_suppresses_control_codes();
    test_arrow_left_sends_ansi_sequence();
    test_arrow_right_sends_ansi_sequence();
    test_arrow_up_and_down_send_ansi_sequences();
    test_escape_and_control_keys();

    TEST_SUMMARY();
    return tests_failed != 0;
}