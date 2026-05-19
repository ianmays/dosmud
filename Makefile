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
	$(MAKE) test-run

# deterministic
test:
	$(CC) $(TEST_CFLAGS) -o $(BIN) $(TEST_SRC)

# deterministic tests (see docs/testing.md for snapshot list)
SNAPSHOT_TESTS = \
	smoke \
	bandit_handover bandit_wielded_give area_items map equipment craft_wielded \
	walk_north walk_map wait_tick \
	frog_replies watchman_talk herbalist_talk archivist_talk talk_nobody \
	use_salve use_torch use_spear use_stone eat_berry eat_fish eat_not_edible eat_missing \
	inspect_rustle inspect_creak inspect_water inspect_grit inspect_none inspect_wrong \
	combat_defend combat_salve combat_no_salve combat_invalid combat_take_blocked \
	combat_victory_xp level_up \
	loot_spear loot_stick loot_berry loot_herb loot_fish loot_empty loot_stripped loot_bag_full \
	bandit_fight bandit_intimidate_ok bandit_intimidate_fail bandit_bag_empty \
	unknown_cmd cannot_move give_wrong_context reply_nobody reply_invalid \
	craft_salve craft_unknown take_nothing take_wrong_item

test-run: test
	@set -e; \
	n=0; \
	for t in $(SNAPSHOT_TESTS); do \
		echo "snapshot: $$t"; \
		./$(BIN) < tests/$$t.input > tests/$$t.output; \
		diff -u tests/$$t.expect tests/$$t.output; \
		n=$$((n + 1)); \
	done; \
	echo "snapshot: seed_cli"; \
	./$(BIN) --seed 1234 < tests/smoke.input > tests/seed_cli.output; \
	diff -u tests/seed_cli.expect tests/seed_cli.output; \
	n=$$((n + 1)); \
	echo "snapshot tests passed: $$n"

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
