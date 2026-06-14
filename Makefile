CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11

TARGET = zerminal
SRC = shell.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: clean