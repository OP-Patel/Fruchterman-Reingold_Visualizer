SRC_DIR = src
BUILD_DIR = build/debug
CC = clang
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_NAME = play
INCLUDE_PATHS = -Iinclude
LIBRARY_PATHS = -L/usr/local/lib
COMPILER_FLAGS = -std=c11 -Wall -O0 -g
LINKER_FLAGS = -lSDL2

all:
	$(CC) $(COMPILER_FLAGS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(SRC_FILES) $(LINKER_FLAGS) -o $(BUILD_DIR)/$(OBJ_NAME)
