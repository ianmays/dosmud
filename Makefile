CC ?= gcc

BASE_CFLAGS = -Wall -Wextra -Wshadow -Wstrict-prototypes -Wmissing-prototypes -std=c89 -pedantic -Iinclude -Isrc
DEV_CFLAGS = $(BASE_CFLAGS)
TEST_CFLAGS = $(BASE_CFLAGS) -Werror -DTEST_MODE

SRC = src/main.c src/game.c src/command.c src/world.c

BIN = dosmud

# default build (normal gameplay)
all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(DEV_CFLAGS) -o $(BIN) $(SRC)

# test build (overwrites same binary)
test:
	$(CC) $(TEST_CFLAGS) -o $(BIN) $(SRC)

# run deterministic test
test-run: test
	./$(BIN) < tests/input.txt > tests/output.txt

clean:
	rm -f $(BIN)

.PHONY: all test test-run clean