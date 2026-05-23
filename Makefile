CC ?= gcc

BASE_CFLAGS = -Wall -Wextra -Wshadow -Wstrict-prototypes -Wmissing-prototypes -std=c89 -pedantic -Iinclude -Isrc
TEST_MODE_FLAG = TEST_MODE
HARNESS_DIR = tests/harness
TEST_CFLAGS = $(BASE_CFLAGS) -Werror -D$(TEST_MODE_FLAG) -g -O0 -I$(HARNESS_DIR)
SRC = src/main.c src/platpos.c src/game.c src/gprog.c src/combat.c src/genc.c src/wanderer.c src/dialogue.c src/gatmos.c src/grendr.c src/invent.c src/command.c src/world.c src/items.c src/txtres.c
HARNESS_SRC = $(HARNESS_DIR)/testharn.c $(HARNESS_DIR)/th_world.c
TEST_SRC = $(SRC) $(HARNESS_SRC)
BIN = dosmud
REGRESSION_DIR = tests/regression
UNIT_DIR = tests/unit

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
	$(MAKE) test-unit-coverage
	$(MAKE) test-soak

# deterministic
test:
	$(CC) $(TEST_CFLAGS) -o $(BIN) $(TEST_SRC)

# deterministic tests (see docs/testing.md for snapshot list)
SNAPSHOT_TESTS = \
	smoke \
	bandit_handover bandit_wielded_give area_items map equipment craft_wielded \
	walk_north walk_map wait_tick \
	frog_replies watchman_talk wanderer_replies wanderer_talk_blocked herbalist_talk archivist_talk talk_nobody \
	use_salve use_torch use_spear use_stone eat_berry eat_fish eat_berry_heal eat_fish_heal \
	eat_not_edible eat_missing \
	inspect_rustle inspect_creak inspect_water inspect_grit inspect_none inspect_wrong \
	combat_defend combat_salve combat_salve_full combat_no_salve combat_invalid combat_take_blocked \
	combat_victory_xp level_up \
	loot_spear loot_stick loot_berry loot_herb loot_fish loot_empty loot_stripped loot_bag_full \
	bandit_fight bandit_intimidate_ok bandit_intimidate_fail bandit_bag_empty \
	unknown_cmd cannot_move give_wrong_context reply_nobody reply_invalid \
	craft_salve craft_unknown take_nothing take_wrong_item

test-run: test
	@set -e; \
	n=0; \
	total=$$(($$(echo $(SNAPSHOT_TESTS) | wc -w) + 1)); \
	for t in $(SNAPSHOT_TESTS); do \
		echo "snapshot: $$t"; \
		./$(BIN) < $(REGRESSION_DIR)/$$t.input > $(REGRESSION_DIR)/$$t.output; \
		diff -u $(REGRESSION_DIR)/$$t.expect $(REGRESSION_DIR)/$$t.output; \
		n=$$((n + 1)); \
	done; \
	echo "snapshot: seed_cli"; \
	./$(BIN) --seed 1234 < $(REGRESSION_DIR)/smoke.input > $(REGRESSION_DIR)/seed_cli.output; \
	diff -u $(REGRESSION_DIR)/seed_cli.expect $(REGRESSION_DIR)/seed_cli.output; \
	n=$$((n + 1)); \
	echo "snapshot tests passed: $$n/$$total"

# gameplay .c files must not call printf (use grendr render_* instead)
check-layers:
	@violators=$$(grep -l 'printf' src/*.c 2>/dev/null | grep -vE '/(main|grendr|platdos|platpos)\.c$$' || true); \
	if [ -n "$$violators" ]; then \
		echo "layer violation: printf only allowed in main.c, grendr.c, platdos.c, platpos.c"; \
		echo "$$violators"; \
		exit 1; \
	fi

# Unit tests (greatest, TEST_MODE, not linked into release dosmud)
UNIT_BUILD_DIR = tests/unit/build
UNIT_COVERAGE_DIR = $(UNIT_BUILD_DIR)/coverage
UNIT_BIN = $(UNIT_BUILD_DIR)/dosmud_unit
UNIT_CFLAGS = $(TEST_CFLAGS) -I$(UNIT_DIR) -fprofile-arcs -ftest-coverage
UNIT_GAMEPLAY_SRC = src/platpos.c src/game.c src/gprog.c src/combat.c src/genc.c \
	src/wanderer.c src/dialogue.c src/gatmos.c src/grendr.c src/invent.c \
	src/command.c src/world.c src/items.c src/txtres.c
UNIT_CORE_SRC = $(UNIT_GAMEPLAY_SRC) $(HARNESS_SRC)
UNIT_TEST_SRC = $(UNIT_DIR)/unit_main.c $(UNIT_DIR)/unit_util.c $(UNIT_DIR)/unit_item.c \
	$(UNIT_DIR)/unit_cmd.c $(UNIT_DIR)/unit_harn.c $(UNIT_DIR)/unit_inv.c $(UNIT_DIR)/unit_cbt.c \
	$(UNIT_DIR)/unit_gprog.c $(UNIT_DIR)/unit_genc.c $(UNIT_DIR)/unit_dial.c $(UNIT_DIR)/unit_wandr.c \
	$(UNIT_DIR)/unit_gatmos.c $(UNIT_DIR)/unit_wrld.c $(UNIT_DIR)/unit_game.c $(UNIT_DIR)/unit_tharn.c
UNIT_CORE_OBJS = $(addprefix $(UNIT_BUILD_DIR)/,$(notdir $(UNIT_CORE_SRC:.c=.o)))
UNIT_TEST_OBJS = $(addprefix $(UNIT_BUILD_DIR)/,$(notdir $(UNIT_TEST_SRC:.c=.o)))
COVERAGE_MODULES = command invent combat game genc wanderer dialogue gatmos world gprog items testharn

ifeq ($(UNIT_BUILD_VERBOSE),1)
UNIT_CC_QUIET =
UNIT_LINK_ANNOUNCE =
else
UNIT_CC_QUIET = @
UNIT_LINK_ANNOUNCE = @echo "building $(UNIT_BIN)..."
endif

$(UNIT_BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(UNIT_BUILD_DIR)
	$(UNIT_CC_QUIET)$(CC) $(UNIT_CFLAGS) -c $< -o $@

$(UNIT_BUILD_DIR)/%.o: $(UNIT_DIR)/%.c
	@mkdir -p $(UNIT_BUILD_DIR)
	$(UNIT_CC_QUIET)$(CC) $(UNIT_CFLAGS) -c $< -o $@

$(UNIT_BUILD_DIR)/%.o: $(HARNESS_DIR)/%.c
	@mkdir -p $(UNIT_BUILD_DIR)
	$(UNIT_CC_QUIET)$(CC) $(UNIT_CFLAGS) -c $< -o $@

$(UNIT_BIN): $(UNIT_CORE_OBJS) $(UNIT_TEST_OBJS)
	@mkdir -p $(UNIT_BUILD_DIR)
	$(UNIT_LINK_ANNOUNCE)
	$(UNIT_CC_QUIET)$(CC) $(UNIT_CFLAGS) -o $@ $(UNIT_CORE_OBJS) $(UNIT_TEST_OBJS)

test-unit: $(UNIT_BIN)
	./$(UNIT_BIN)

test-unit-verbose: $(UNIT_BIN)
	./$(UNIT_BIN) --verbose

test-unit-verbose-gameplay: $(UNIT_BIN)
	./$(UNIT_BIN) --verbose-gameplay

test-unit-coverage: test-unit
	@mkdir -p $(UNIT_COVERAGE_DIR)
	@echo "unit coverage (branch % / line %):"
	@rm -f $(UNIT_COVERAGE_DIR)/.cov_rows; \
	below=""; \
	for f in $(COVERAGE_MODULES); do \
		if [ "$$f" = "testharn" ]; then \
			covsrc=$(CURDIR)/$(HARNESS_DIR)/testharn.c; \
		else \
			covsrc=$(CURDIR)/src/$$f.c; \
		fi; \
		stats=$$(cd $(UNIT_COVERAGE_DIR) && gcov -b -o $(CURDIR)/$(UNIT_BUILD_DIR) $$covsrc 2>/dev/null | awk ' \
			/^Lines executed:/ && !seen_l { seen_l=1; split($$2,a,":"); lnp=a[2]; gsub(/%/,"",lnp); lt=$$4+0; le=lt*lnp/100 } \
			/^Branches executed:/ && !seen_b { seen_b=1; split($$2,a,":"); brp=a[2]; gsub(/%/,"",brp); bt=$$4+0; be=bt*brp/100 } \
			END { if (seen_l && seen_b) printf "%s %s %.6f %.0f %.6f %.0f\n", brp, lnp, le, lt, be, bt }'); \
		br=$$(echo $$stats | awk '{print $$1}'); \
		ln=$$(echo $$stats | awk '{print $$2}'); \
		printf "  %-10s %6s / %6s\n" "$$f" "$$br" "$$ln"; \
		printf '%s\n' "$$stats" >> $(UNIT_COVERAGE_DIR)/.cov_rows; \
		if echo "$$br" | awk '{ exit !($$1+0 < 90) }'; then below="$$below $$f"; fi; \
	done; \
	echo "  -------"; \
	awk '{ le+=$$3; lt+=$$4; be+=$$5; bt+=$$6 } END { \
		if (lt > 0 && bt > 0) printf "  %-10s %6.2f / %6.2f\n", "overall", 100*be/bt, 100*le/lt }' \
		$(UNIT_COVERAGE_DIR)/.cov_rows; \
	rm -f $(UNIT_COVERAGE_DIR)/.cov_rows; \
	if [ -n "$$below" ]; then echo "below 90% branch:$$below"; fi

test-unit-coverage-verbose:
	$(MAKE) UNIT_BUILD_VERBOSE=1 test-unit
	@mkdir -p $(UNIT_COVERAGE_DIR)
	@for f in $(COVERAGE_MODULES); do \
		echo "=== $$f ==="; \
		if [ "$$f" = "testharn" ]; then \
			covsrc=$(CURDIR)/$(HARNESS_DIR)/testharn.c; \
		else \
			covsrc=$(CURDIR)/src/$$f.c; \
		fi; \
		(cd $(UNIT_COVERAGE_DIR) && gcov -b -o $(CURDIR)/$(UNIT_BUILD_DIR) $$covsrc 2>/dev/null | grep -E '^File|^Lines|^Branches') || true; \
	done

# Soak / stress tests (separate binary; keep UNIT_GAMEPLAY_SRC in sync with gameplay objects above)
SOAK_DIR = tests/soak
SOAK_BUILD_DIR = $(SOAK_DIR)/build
SOAK_BIN = $(SOAK_BUILD_DIR)/dosmud_soak
SOAK_CFLAGS = $(TEST_CFLAGS) -I$(SOAK_DIR) -I$(UNIT_DIR)
SOAK_CORE_SRC = $(UNIT_GAMEPLAY_SRC) $(HARNESS_DIR)/th_world.c
SOAK_TEST_SRC = $(SOAK_DIR)/soak_main.c $(SOAK_DIR)/soak_sim.c $(SOAK_DIR)/soak_util.c \
	$(UNIT_DIR)/unit_util.c
SOAK_CORE_OBJS = $(addprefix $(SOAK_BUILD_DIR)/,$(notdir $(SOAK_CORE_SRC:.c=.o)))
SOAK_TEST_OBJS = $(addprefix $(SOAK_BUILD_DIR)/,$(notdir $(SOAK_TEST_SRC:.c=.o)))

$(SOAK_BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(SOAK_BUILD_DIR)
	@$(CC) $(SOAK_CFLAGS) -c $< -o $@

$(SOAK_BUILD_DIR)/%.o: $(SOAK_DIR)/%.c
	@mkdir -p $(SOAK_BUILD_DIR)
	@$(CC) $(SOAK_CFLAGS) -c $< -o $@

$(SOAK_BUILD_DIR)/%.o: $(HARNESS_DIR)/%.c
	@mkdir -p $(SOAK_BUILD_DIR)
	@$(CC) $(SOAK_CFLAGS) -c $< -o $@

$(SOAK_BUILD_DIR)/unit_util.o: $(UNIT_DIR)/unit_util.c
	@mkdir -p $(SOAK_BUILD_DIR)
	@$(CC) $(SOAK_CFLAGS) -c $< -o $@

$(SOAK_BIN): $(SOAK_CORE_OBJS) $(SOAK_TEST_OBJS)
	@mkdir -p $(SOAK_BUILD_DIR)
	@echo "building $(SOAK_BIN)..."
	@$(CC) $(SOAK_CFLAGS) -o $@ $(SOAK_CORE_OBJS) $(SOAK_TEST_OBJS)

test-soak: $(SOAK_BIN)
	./$(SOAK_BIN)

clean:
	rm -f $(BIN)
	rm -rf $(UNIT_BUILD_DIR)
	rm -rf $(SOAK_BUILD_DIR)
	rm -f $(REGRESSION_DIR)/*.output tests/*.output
	rm -f dosmud_unit dosmud_unit-*.gcno dosmud_unit-*.gcda *.gcov src/*.gcno src/*.gcda

dos-prepare:
	powershell.exe -ExecutionPolicy Bypass -File dos-prepare.ps1 $(if $(MODE),-Mode $(MODE))

dos-run:
	powershell.exe -ExecutionPolicy Bypass -File dos-prepare.ps1 -NoBuild

.PHONY: build-all build test-all test test-run test-unit test-unit-verbose test-unit-verbose-gameplay test-unit-coverage test-unit-coverage-verbose test-soak check-layers clean dos-prepare dos-run
