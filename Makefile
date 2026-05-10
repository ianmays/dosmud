CC ?= gcc

BASE_CFLAGS = -Wall -Wextra -Wshadow -Wstrict-prototypes -Wmissing-prototypes -std=c89 -pedantic -Iinclude -Isrc
DEV_CFLAGS = $(BASE_CFLAGS)
TEST_MODE_FLAG = TEST_MODE
TEST_CFLAGS = $(BASE_CFLAGS) -Werror -D$(TEST_MODE_FLAG)
SRC = src/main.c src/game.c src/grendr.c src/invent.c src/command.c src/world.c src/items.c src/txtres.c
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
test-run:
	./$(BIN) < tests/smoke.input > tests/smoke.output
	diff -u \
		tests/smoke.expect \
		tests/smoke.output
	./$(BIN) < tests/bandit_handover.input > tests/bandit_handover.output
	diff -u \
		tests/bandit_handover.expect \
		tests/bandit_handover.output
	./$(BIN) < tests/area_items.input > tests/area_items.output
	diff -u \
		tests/area_items.expect \
		tests/area_items.output
	./$(BIN) < tests/ascii_art_gameplay.input > tests/ascii_art_gameplay.output
	diff -u \
		tests/ascii_art_gameplay.expect \
		tests/ascii_art_gameplay.output

clean:
	rm -f $(BIN)

prepare-dos:
	powershell.exe -ExecutionPolicy Bypass -File prepare-dos.ps1 $(if $(MODE),-Mode $(MODE))

.PHONY: all build all-test test test-run clean prepare-dos