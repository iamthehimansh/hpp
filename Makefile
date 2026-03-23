CC = gcc
CFLAGS ?= -Wall -Wextra -Werror -std=c11 -g
LDFLAGS =

SRC_DIR = compiler
BUILD_DIR = build
TARGET = $(BUILD_DIR)/hpp

# Find all .c files under compiler/
SRCS = $(shell find $(SRC_DIR) -name '*.c')
# Mirror source tree in build dir
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)

test: $(TARGET)
	@bash $(SRC_DIR)/tests/run_tests.sh
