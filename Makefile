CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11

TARGET = bin/zerminal
SRC = src/shell.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: clean