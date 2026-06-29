CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -pedantic -g
INCLUDES = -Iinclude

SRCS = $(shell find src -type f -name "*.c")
OBJS = $(SRCS:src/%.c=build/%.o)
TARGET = build/speakc

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

build/%.o: src/%.c | build
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build

.PHONY: all clean
	