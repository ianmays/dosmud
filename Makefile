CC ?= gcc

BASE_CFLAGS = -Wall -Wextra -Wshadow -Wstrict-prototypes -Wmissing-prototypes -std=c89 -pedantic -Iinclude -Isrc
TEST_MODE_FLAG = TEST_MODE
TEST_CFLAGS = $(BASE_CFLAGS) -Werror -D$(TEST_MODE_FLAG) -g -O0
SRC = src/main.c src/platpos.c src/game.c src/gprog.c src/combat.c src/genc.c src/wanderer.c src/dialogue.c src/gatmos.c src/grendr.c src/invent.c src/command.c src/world.c src/items.c src/txtres.c
TEST_SRC = $(SRC) src/testharn.c
BIN = dosmud

build-all:
	$(MAKE) clean
	$(MAKE) dos-prepare
	$(MAKE) build

build:
	$(CC) $(BASE_CFLAGS) -o $(BIN) $(SRC)

# deterministic
test-all:
	$(MAKE) check-layers
	$(MAKE) clean
	$(MAKE) dos-prepare MODE=$(TEST_MODE_FLAG)
	$(MAKE) test
	$(MAKE) test-run

# deterministic
test:
	$(CC) $(TEST_CFLAGS) -o $(BIN) $(TEST_SRC)

# deterministic tests
test-run:
	./$(BIN) < tests/smoke.input > tests/smoke.output
	diff -u \
		tests/smoke.expect \
		tests/smoke.output
	./$(BIN) --seed 1234 < tests/smoke.input > tests/seed_cli.output
	diff -u \
		tests/seed_cli.expect \
		tests/seed_cli.output
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
	@violators=$$(grep -l 'printf' src/*.c 2>/dev/null | grep -vE '/(main|grendr|platdos|platpos)\.c$$' || true); \
	if [ -n "$$violators" ]; then \
		echo "layer violation: printf only allowed in main.c, grendr.c, platdos.c, platpos.c"; \
		echo "$$violators"; \
		exit 1; \
	fi

clean:
	rm -f $(BIN)

dos-prepare:
	powershell.exe -ExecutionPolicy Bypass -File dos-prepare.ps1 $(if $(MODE),-Mode $(MODE))

dos-run:
	powershell.exe -ExecutionPolicy Bypass -File dos-prepare.ps1 -NoBuild

.PHONY: build-all build test-all test test-run check-layers clean dos-prepare dos-run
