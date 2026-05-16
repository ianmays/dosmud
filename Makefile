CC ?= gcc

BASE_CFLAGS = -Wall -Wextra -Wshadow -Wstrict-prototypes -Wmissing-prototypes -std=c89 -pedantic -Iinclude -Isrc
TEST_MODE_FLAG = TEST_MODE
TEST_CFLAGS = $(BASE_CFLAGS) -Werror -D$(TEST_MODE_FLAG) -g -O0
SRC = src/main.c src/game.c src/gprog.c src/combat.c src/genc.c src/wanderer.c src/dialogue.c src/gatmos.c src/grendr.c src/invent.c src/command.c src/world.c src/items.c src/txtres.c
BIN = dosmud

all-build: $(BIN)
$(BIN): $(SRC)
	$(MAKE) clean && $(MAKE) prepare-dos && $(CC) $(BASE_CFLAGS) -o $(BIN) $(SRC)

build:
	$(MAKE) clean && $(CC) $(BASE_CFLAGS) -o $(BIN) $(SRC)

# deterministic
all-test: check-layers
	$(MAKE) clean && $(MAKE) prepare-dos MODE=$(TEST_MODE_FLAG) && $(CC) $(TEST_CFLAGS) -o $(BIN) $(SRC)

# deterministic
test: check-layers
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
	./$(BIN) < tests/map.input > tests/map.output
	diff -u \
		tests/map.expect \
		tests/map.output
	./$(BIN) < tests/equipment.input > tests/equipment.output
	diff -u \
		tests/equipment.expect \
		tests/equipment.output
	./$(BIN) < tests/bandit_wielded_give.input > tests/bandit_wielded_give.output
	diff -u \
		tests/bandit_wielded_give.expect \
		tests/bandit_wielded_give.output
	./$(BIN) < tests/craft_wielded.input > tests/craft_wielded.output
	diff -u \
		tests/craft_wielded.expect \
		tests/craft_wielded.output

# gameplay .c files must not call printf (use grendr render_* instead)
check-layers:
	@violators=$$(grep -l 'printf' src/*.c 2>/dev/null | grep -vE '/(main|grendr)\.c$$' || true); \
	if [ -n "$$violators" ]; then \
		echo "layer violation: printf only allowed in main.c and grendr.c"; \
		echo "$$violators"; \
		exit 1; \
	fi

clean:
	rm -f $(BIN)

prepare-dos:
	powershell.exe -ExecutionPolicy Bypass -File prepare-dos.ps1 $(if $(MODE),-Mode $(MODE))

run-dos:
	powershell.exe -ExecutionPolicy Bypass -File prepare-dos.ps1 -NoBuild

.PHONY: all build all-test test test-run check-layers clean prepare-dos run-dos