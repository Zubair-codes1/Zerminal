CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11

ZERMINAL = bin/zerminal
ZERMINAL_SRC = src/shell.c

MANAGER = bin/manager
MANAGER_SRC = src/manager.c

all: $(ZERMINAL) $(MANAGER)

$(ZERMINAL): $(ZERMINAL_SRC)
	$(CC) $(CFLAGS) -o $@ $<

$(MANAGER): $(MANAGER_SRC)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(ZERMINAL) $(MANAGER)

.PHONY: all clean