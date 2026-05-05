# Host-only sanity check (GCC). Release DOS build: see build_dos.bat + Open Watcom.
CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror -Wshadow -std=c89 -pedantic -Iinclude -Isrc 

SRC = src/main.c src/game.c src/command.c src/world.c
BIN = dosmud

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $(BIN) $(SRC)

clean:
	rm -f $(BIN)

.PHONY: all clean
