# Compiler & Flags
CC          := clang
CFLAGS      := -Wall -Wextra -std=c11 -pedantic -g
INCLUDES    := -Iinclude -I/opt/homebrew/include
LDFLAGS     := -L/opt/homebrew/lib -lraylib -lpthread

# Directories
SRC_DIR     := src
BIN_DIR     := bin
TEST_DIR    := tests

# Source Files
MANAGER_SRC := $(SRC_DIR)/manager.c
SCREEN_SRC  := $(SRC_DIR)/screen.c
SHELL_SRC   := $(SRC_DIR)/shell.c

# Executables
TARGET      := $(BIN_DIR)/terminal
SHELL_BIN   := $(BIN_DIR)/shell

# Test Executables
TEST_INPUT  := $(BIN_DIR)/test_input
TEST_SCREEN := $(BIN_DIR)/test_screen
TEST_SHELL  := $(BIN_DIR)/test_shell

.PHONY: all clean tests run

# Default target: build main terminal and shell
all: $(TARGET) $(SHELL_BIN)

# Build main terminal application (manager + screen)
$(TARGET): $(MANAGER_SRC) $(SCREEN_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

# Build standalone shell binary
$(SHELL_BIN): $(SHELL_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

# Test Binaries (links screen.c only, avoiding duplicate main symbols)
$(TEST_INPUT): $(TEST_DIR)/test_input.c $(SCREEN_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

$(TEST_SCREEN): $(TEST_DIR)/test_screen.c $(SCREEN_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

$(TEST_SHELL): $(TEST_DIR)/test_shell.c $(SCREEN_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

# Run all 3 test suites
tests: $(SHELL_BIN) $(TEST_INPUT) $(TEST_SCREEN) $(TEST_SHELL)	
	@echo "\n=== Running test_input ==="
	@./$(TEST_INPUT)
	@echo "\n=== Running test_screen ==="
	@./$(TEST_SCREEN)
	@echo "\n=== Running test_shell ==="
	@./$(TEST_SHELL)

# Ensure output directory exists
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Run the primary terminal app
run: $(TARGET)
	./$(TARGET)

# Clean up build artifacts
clean:
	rm -rf $(BIN_DIR)