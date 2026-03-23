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

PREFIX ?= /usr/local

.PHONY: all clean test install uninstall

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

install: $(TARGET)
	@mkdir -p $(PREFIX)/bin
	@cp $(TARGET) $(PREFIX)/bin/hpp
	@mkdir -p $(PREFIX)/lib/hpp/stdlib/std
	@cp $(SRC_DIR)/stdlib/std_ll.asm $(PREFIX)/lib/hpp/stdlib/
	@cp $(SRC_DIR)/stdlib/std_util.asm $(PREFIX)/lib/hpp/stdlib/
	@cp $(SRC_DIR)/stdlib/std/*.asm $(PREFIX)/lib/hpp/stdlib/std/ 2>/dev/null || true
	@cp $(SRC_DIR)/stdlib/std/*.hdef $(PREFIX)/lib/hpp/stdlib/std/
	@mkdir -p $(PREFIX)/share/man/man1 $(PREFIX)/share/man/man3 $(PREFIX)/share/man/man7
	@cp man/man1/hpp.1 $(PREFIX)/share/man/man1/
	@cp man/man3/hpp-stdlib.3 $(PREFIX)/share/man/man3/
	@cp man/man7/hpp-lang.7 $(PREFIX)/share/man/man7/
	@echo "Installed hpp to $(PREFIX)/bin/hpp"
	@echo "Run 'hpp --help' to get started"

uninstall:
	@rm -f $(PREFIX)/bin/hpp
	@rm -rf $(PREFIX)/lib/hpp
	@rm -f $(PREFIX)/share/man/man1/hpp.1
	@rm -f $(PREFIX)/share/man/man3/hpp-stdlib.3
	@rm -f $(PREFIX)/share/man/man7/hpp-lang.7
	@echo "Uninstalled hpp"
