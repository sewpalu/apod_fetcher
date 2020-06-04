CC = gcc
CC_FLAGS = --std=c2x -O3 -Wall -Wextra -Werror -ggdb
LINKER_FLAGS = -lcurl -lgumbo
BIN_NAME = apod_fetch
BIN_DIR = .bin
INSTALL_PREFIX = /usr
BIN_INSTALL_DIR = $(INSTALL_PREFIX)/bin

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

build: $(BIN_DIR) main.o
	$(CC) -o $(BIN_DIR)/$(BIN_NAME) $(BIN_DIR)/main.o $(CC_FLAGS) $(LINKER_FLAGS)

%.o: %.c
	$(CC) -o $(BIN_DIR)/$@ -c $< $(CC_FLAGS)

run: build
	$(BIN_DIR)/$(BIN_NAME) wallpaper

install: build
	cp $(BIN_DIR)/$(BIN_NAME) $(BIN_INSTALL_DIR)/$(BIN_NAME)

clean:
	rm -rf $(BIN_DIR)/*

rebuild: clean build

gdb: build
	gdb --args $(BIN_DIR)/$(BIN_NAME) gdb_wallpaper

