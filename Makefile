# Compiler & Flags
CC          := clang
CFLAGS      := -Wall -Wextra -std=c11 -pedantic -g
INCLUDES    := -Iinclude -I/opt/homebrew/include
LDFLAGS     := -L/opt/homebrew/lib -lraylib -lpthread

# Directories
SRC_DIR     := src
BUILD_DIR   := build
BIN_DIR     := bin
TEST_DIR    := tests

# Source Files & Objects
SRCS        := $(wildcard $(SRC_DIR)/*.c)
OBJS        := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Executables
TARGET      := $(BIN_DIR)/terminal
SHELL_BIN   := $(BIN_DIR)/shell

# Test Executables
TEST_SCREEN := $(BIN_DIR)/test_screen
TEST_SHELL  := $(BIN_DIR)/test_shell

.PHONY: all clean tests run

# Default target: build the main app & shell
all: $(TARGET) $(SHELL_BIN)

# Build main application
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

# Build shell application (if built from src/shell.c or standalone)
$(SHELL_BIN): $(SRC_DIR)/shell.c | $(BIN_DIR)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

# Compile object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Build and run all tests
tests: $(TEST_SCREEN) $(TEST_SHELL)
	@echo "--- Running test_screen ---"
	@./$(TEST_SCREEN)
	@echo "--- Running test_shell ---"
	@./$(TEST_SHELL)

# Test Screen binary (links screen implementation)
$(TEST_SCREEN): $(TEST_DIR)/test_screen.c $(SRC_DIR)/screen.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

# Test Shell binary
$(TEST_SHELL): $(TEST_DIR)/test_shell.c $(SRC_DIR)/screen.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

# Create output directories
$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

# Run main terminal
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)