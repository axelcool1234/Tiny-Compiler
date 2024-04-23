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
TESTS = compilertest_main
EXECUTABLE = compiler

# all: setup $(EXECUTABLE)

tests: setup $(TESTS)

debug: CFLAGS += $(DFLAGS)
debug: all

$(EXECUTABLE): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@

$(TESTS): $(OBJS)
	$(CC) $(CFLAGS) $^ tests/compilertest_main.cpp -o $(BIN_DIR)/$@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) -c -I $(INCLUDE_DIR) $(CFLAGS) $< -o $@

setup:
	@mkdir -p $(BIN_DIR) $(BUILD_DIR)

clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR) $(CACHE_DIR) $(BEAR_FILE)

.PHONY: clean
