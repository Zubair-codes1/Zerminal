CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11
TEST_CFLAGS = $(CFLAGS) -Iinclude

SHELL_BIN = bin/shell
SHELL_SRC = src/shell.c

ZERMINAL_BIN = bin/zerminal
ZERMINAL_SRC = src/manager.c src/screen.c

TEST_SCREEN_BIN = bin/test_screen
TEST_SHELL_BIN = bin/test_shell

all: $(SHELL_BIN) $(ZERMINAL_BIN)

$(SHELL_BIN): $(SHELL_SRC) | bin
	$(CC) $(CFLAGS) -o $@ $< -lpthread

$(ZERMINAL_BIN): $(ZERMINAL_SRC) | bin
	$(CC) -I/opt/homebrew/include $(CFLAGS) -o $@ $^ -L/opt/homebrew/lib -lpthread -lraylib

# --- Tests ---

test: test-screen test-shell

test-screen: $(TEST_SCREEN_BIN)
	./$(TEST_SCREEN_BIN)

$(TEST_SCREEN_BIN): tests/test_screen.c src/screen.c | bin
	$(CC) $(TEST_CFLAGS) -o $@ $^ -lpthread

# test_shell.c execs $(SHELL_BIN) as a real subprocess, so the shell
# binary must be built first.
test-shell: $(SHELL_BIN) $(TEST_SHELL_BIN)
	./$(TEST_SHELL_BIN)

$(TEST_SHELL_BIN): tests/test_shell.c | bin
	$(CC) $(TEST_CFLAGS) -o $@ $<

bin:
	mkdir -p bin

clean:
	rm -f $(SHELL_BIN) $(ZERMINAL_BIN) $(TEST_SCREEN_BIN) $(TEST_SHELL_BIN)

.PHONY: all clean test test-screen test-shell