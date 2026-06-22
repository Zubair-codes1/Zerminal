CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11

SHELL_BIN = bin/shell
SHELL_SRC = src/shell.c

MANAGER_BIN = bin/manager
MANAGER_SRC = src/manager.c

all: $(SHELL_BIN) $(MANAGER_BIN)

$(SHELL_BIN): $(SHELL_SRC)
	$(CC) $(CFLAGS) -o $@ $<

$(MANAGER_BIN): $(MANAGER_SRC)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(SHELL_BIN) $(MANAGER_BIN)

.PHONY: all clean