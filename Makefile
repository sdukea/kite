CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -g -I$(SRC_DIR)

SRC_DIR = src
BUILD_DIR = build
TARGET = kite

SRCS = $(SRC_DIR)/tree.c $(SRC_DIR)/filesystem.c $(SRC_DIR)/commands.c $(SRC_DIR)/persistence.c $(SRC_DIR)/main.c
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

TEST_SRC = tests/test_kite.c
TEST_TARGET = $(BUILD_DIR)/test_kite

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRC) $(SRC_DIR)/tree.c $(SRC_DIR)/filesystem.c $(SRC_DIR)/persistence.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_SRC) $(SRC_DIR)/tree.c $(SRC_DIR)/filesystem.c $(SRC_DIR)/persistence.c

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
