CC ?= gcc

BASE_CFLAGS = -Wall -Wextra -Wshadow -Wstrict-prototypes -Wmissing-prototypes -std=c89 -pedantic -Iinclude -Isrc
DEV_CFLAGS = $(BASE_CFLAGS)
TEST_MODE_FLAG = TEST_MODE
TEST_CFLAGS = $(BASE_CFLAGS) -Werror -D$(TEST_MODE_FLAG)
SRC = src/main.c src/game.c src/grendr.c src/invent.c src/command.c src/world.c src/items.c
BIN = dosmud

all-build: $(BIN)
$(BIN): $(SRC)
	$(MAKE) clean && $(MAKE) prepare-dos && $(CC) $(DEV_CFLAGS) -o $(BIN) $(SRC)

build:
	$(MAKE) clean && $(CC) $(DEV_CFLAGS) -o $(BIN) $(SRC)

# deterministic
all-test:
	$(MAKE) clean && $(MAKE) prepare-dos MODE=$(TEST_MODE_FLAG) && $(CC) $(TEST_CFLAGS) -o $(BIN) $(SRC)

# deterministic
test:
	$(MAKE) clean && $(CC) $(TEST_CFLAGS) -o $(BIN) $(SRC)

# deterministic tests
test-run: test
	./$(BIN) < tests/input.txt > tests/output.txt

clean:
	rm -f $(BIN)

prepare-dos:
	powershell.exe -ExecutionPolicy Bypass -File prepare-dos.ps1 $(if $(MODE),-Mode $(MODE))

.PHONY: all build all-test test test-run clean prepare-dos