CC:=g++
CFLAGS:=-std=c++23 -Iinclude -Wall -Werror -Wno-unused-variable
DFLAGS := -ggdb -DDEBUG

SRC_DIR:=src
INCLUDE_DIR:=include
BUILD_DIR:=build
BIN_DIR:=bin
CACHE_DIR:=.cache
BEAR_FILE:=compile_commands.json

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))
EXECUTABLE = compiler

all: setup $(EXECUTABLE)

debug: CFLAGS += $(DFLAGS)
debug: all

$(EXECUTABLE): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) -c -I $(INCLUDE_DIR) $(CFLAGS) $< -o $@

setup:
	@mkdir -p $(BIN_DIR) $(BUILD_DIR)

clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR) $(CACHE_DIR) $(BEAR_FILE)

.PHONY: clean
