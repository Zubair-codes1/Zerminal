CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11

SHELL_BIN = bin/shell
SHELL_SRC = src/shell.c

ZERMINAL_BIN = bin/zerminal
ZERMINAL_SRC = src/manager.c src/screen.c

all: $(SHELL_BIN) $(ZERMINAL_BIN)

$(SHELL_BIN): $(SHELL_SRC)
	$(CC) $(CFLAGS) -o $@ $< -lpthread

$(ZERMINAL_BIN): $(ZERMINAL_SRC)
	$(CC) -I/opt/homebrew/include $(CFLAGS) -o $@ $^ -L/opt/homebrew/lib -lpthread -lraylib

clean:
	rm -f $(SHELL_BIN) $(ZERMINAL_BIN)

.PHONY: all clean