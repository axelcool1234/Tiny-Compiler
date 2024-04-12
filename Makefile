CC:=g++
CFLAGS:=-std=c++23 -Iinclude -Wall -Werror -Wno-unused-variable
DFLAGS := -ggdb -DDEBUG

SRC_DIR:=src
INCLUDE_DIR:=include
BIN_DIR:=bin
CACHE_DIR:=.cache
BEAR_FILE:=compile_commands.json

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
EXECUTABLE = compiler

all: setup $(EXECUTABLE)

debug: CFLAGS += $(DFLAGS)
debug: all

$(EXECUTABLE): $(SRCS)
	$(CC) -I $(INCLUDE_DIR) $(CFLAGS) $^ -o $(BIN_DIR)/$@

setup:
	@mkdir -p bin

clean:
	rm -rf $(BIN_DIR) $(CACHE_DIR) $(BEAR_FILE)

.PHONY: clean
