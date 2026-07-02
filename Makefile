CC ?= gcc
WIN_CC ?= x86_64-w64-mingw32-gcc

BUILD_INCLUDE_DIR = build/include
VERSION_HDR = $(BUILD_INCLUDE_DIR)/version.h
BASE_CFLAGS = -Wall -Wextra -Wshadow -Wstrict-prototypes -Wmissing-prototypes -std=c89 -pedantic -I$(BUILD_INCLUDE_DIR) -Iinclude -Isrc
TEST_MODE_FLAG = TEST_MODE
HARNESS_DIR = tests/harness
TEST_CFLAGS = $(BASE_CFLAGS) -Werror -D$(TEST_MODE_FLAG) -g -O0 -I$(HARNESS_DIR)
TARGET ?= posix
BIN ?= dosmud
PLAT_SRC = src/platpos.c
ifeq ($(TARGET),win)
PLAT_SRC = src/platwin.c
endif
SRC = src/main.c $(PLAT_SRC) src/buildid.c src/game.c src/gout.c src/gprog.c src/gstory.c src/gwhok.c src/combat.c src/genc.c src/dialogue.c src/npc.c src/gatmos.c src/grendr.c src/fmt.c src/invent.c src/command.c src/world.c src/items.c src/replay.c src/save.c src/txtres.c
HARNESS_SRC = $(HARNESS_DIR)/testharn.c $(HARNESS_DIR)/th_world.c
TEST_SRC = $(SRC) $(HARNESS_SRC)
REGRESSION_DIR = tests/regression
UNIT_DIR = tests/unit
RUN_ARGS = $(if $(SEED),--seed $(SEED))

define RUN_TIMED
@start=$$(date +%s%3N); \
status=0; \
{ $(1); } || status=$$?; \
end=$$(date +%s%3N); \
elapsed_ms=$$((end - start)); \
elapsed_s=$$((elapsed_ms / 1000)); \
elapsed_rem=$$((elapsed_ms % 1000)); \
printf 'elapsed: %s.%03ds\n' "$$elapsed_s" "$$elapsed_rem"; \
exit $$status
endef

FORCE:

$(VERSION_HDR): FORCE VERSION scripts/gen-version-header.sh
	sh scripts/gen-version-header.sh VERSION $@

build-all:
	$(MAKE) clean
	$(MAKE) dos-prepare NORUN=1
	$(MAKE) build

build: $(VERSION_HDR)
	$(call RUN_TIMED,$(CC) $(BASE_CFLAGS) -o $(BIN) $(SRC))

build-win:
	$(MAKE) TARGET=win CC=$(WIN_CC) BIN=dosmud.exe build

win-run:
	@if [ ! -f dosmud.exe ]; then \
		echo "missing dosmud.exe - run 'make build-win' or 'make test-win' first"; \
		exit 1; \
	fi
	@win_exe=$$(wslpath -w "$(CURDIR)/dosmud.exe"); \
	if [ -n "$(SEED)" ]; then \
		powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath '$$win_exe' -ArgumentList '--seed','$(SEED)'"; \
	else \
		powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath '$$win_exe'"; \
	fi

run: build
	./$(BIN) $(RUN_ARGS)

# deterministic
test-all:
	$(MAKE) check-layers
	$(MAKE) clean
	$(MAKE) dos-prepare MODE=$(TEST_MODE_FLAG) NORUN=1
	$(MAKE) test-run
	$(MAKE) test-unit-coverage
	$(MAKE) test-soak

# deterministic
test: $(VERSION_HDR)
	$(call RUN_TIMED,$(CC) $(TEST_CFLAGS) -o $(BIN) $(TEST_SRC))

test-win:
	$(MAKE) TARGET=win CC=$(WIN_CC) BIN=dosmud.exe test

test-run-bin: test
	./$(BIN) $(RUN_ARGS)

# deterministic tests (see docs/testing.md for snapshot list)
# SNAPSHOT_TESTS: combat_* and level_up exercise generic combat/progression events (#159); bandit_* keeps the roster-backed enemy path visible
SNAPSHOT_TESTS = \
	smoke \
	bandit_handover bandit_wielded_give area_items map equipment craft_wielded \
	walk_north walk_map wait_tick \
	frog_hint frog_replies watchman_talk watchman_warned_followup watchman_meal_peckish watchman_meal_tower_desc traveler_replies traveler_talk_blocked herbalist_talk archivist_talk talk_nobody \
	herbalist_request herbalist_incomplete herbalist_complete herbalist_followup herbalist_save_load \
	herbalist_give_reject herbalist_give_floor \
	dialogue_menu_exit \
	game_event_dialogue narrative_indirection \
	use_salve use_torch use_spear use_stone eat_berry eat_fish eat_berry_heal eat_fish_heal \
	eat_not_edible eat_missing bag_stacks bag_view \
	inspect_rustle inspect_creak inspect_water inspect_grit inspect_none inspect_wrong \
	ambient_rustle ambient_tick_order ambient_item \
	combat_defend combat_salve combat_no_salve combat_invalid combat_take_blocked \
	combat_victory_xp level_up \
	loot_spear loot_stick loot_berry loot_herb loot_fish loot_empty loot_stripped loot_bag_full loot_multi \
	loot_menu_exit \
	loot_all_multi loot_all_bag_full \
	bandit_fight bandit_intimidate_ok bandit_intimidate_fail bandit_bag_empty bandit_road \
	unknown_cmd cannot_move give_wrong_context reply_nobody post_combat_reply_guard reply_invalid \
	craft_salve craft_unknown take_nothing take_wrong_item take_all take_all_bag_full \
	save_load save_load_loot_menu \
	replay_log replay_save_load version

snapshot-run:
	@set -e; \
	n=0; \
	total=$$(($$(echo $(SNAPSHOT_TESTS) | wc -w) + 2)); \
	for t in $(SNAPSHOT_TESTS); do \
		echo "snapshot: $$t"; \
		if [ "$$t" = "replay_log" ]; then \
			./$(BIN) --seed 1234 --replay-log $(REGRESSION_DIR)/replay_log_log.output < $(REGRESSION_DIR)/replay_log.input > $(REGRESSION_DIR)/replay_log.output; \
			diff -u $(REGRESSION_DIR)/replay_log.expect $(REGRESSION_DIR)/replay_log.output; \
			diff -u $(REGRESSION_DIR)/replay_log_log.expect $(REGRESSION_DIR)/replay_log_log.output; \
		elif [ "$$t" = "replay_save_load" ]; then \
			rm -f save.dat; \
			./$(BIN) --seed 1234 --replay-log $(REGRESSION_DIR)/replay_save_load_log.output < $(REGRESSION_DIR)/replay_save_load.input > $(REGRESSION_DIR)/replay_save_load.output; \
			diff -u $(REGRESSION_DIR)/replay_save_load.expect $(REGRESSION_DIR)/replay_save_load.output; \
			diff -u $(REGRESSION_DIR)/replay_save_load_log.expect $(REGRESSION_DIR)/replay_save_load_log.output; \
			rm -f save.dat; \
		elif [ "$$t" = "save_load" ]; then \
			rm -f save.dat; \
			./$(BIN) < $(REGRESSION_DIR)/$$t.input > $(REGRESSION_DIR)/$$t.output; \
			diff -u $(REGRESSION_DIR)/$$t.expect $(REGRESSION_DIR)/$$t.output; \
			rm -f save.dat; \
		elif [ "$$t" = "herbalist_save_load" ]; then \
			rm -f save.dat; \
			./$(BIN) < $(REGRESSION_DIR)/$$t.input > $(REGRESSION_DIR)/$$t.output; \
			diff -u $(REGRESSION_DIR)/$$t.expect $(REGRESSION_DIR)/$$t.output; \
			rm -f save.dat; \
		elif [ "$$t" = "version" ]; then \
			version_string=$$(sed -n 's/^#define BUILD_VERSION_STRING "\(.*\)"/\1/p' $(VERSION_HDR)); \
			sed "s/@VERSION@/$$version_string/g" $(REGRESSION_DIR)/$$t.expect > $(REGRESSION_DIR)/$$t.expect.output; \
			./$(BIN) < $(REGRESSION_DIR)/$$t.input > $(REGRESSION_DIR)/$$t.output; \
			diff -u $(REGRESSION_DIR)/$$t.expect.output $(REGRESSION_DIR)/$$t.output; \
		else \
			./$(BIN) < $(REGRESSION_DIR)/$$t.input > $(REGRESSION_DIR)/$$t.output; \
			diff -u $(REGRESSION_DIR)/$$t.expect $(REGRESSION_DIR)/$$t.output; \
		fi; \
		n=$$((n + 1)); \
	done; \
	echo "snapshot: seed_cli"; \
	./$(BIN) --seed 1234 < $(REGRESSION_DIR)/smoke.input > $(REGRESSION_DIR)/seed_cli.output; \
	diff -u $(REGRESSION_DIR)/seed_cli.expect $(REGRESSION_DIR)/seed_cli.output; \
	n=$$((n + 1)); \
	echo "snapshot: version_cli"; \
	version_string=$$(sed -n 's/^#define BUILD_VERSION_STRING "\(.*\)"/\1/p' $(VERSION_HDR)); \
	sed "s/@VERSION@/$$version_string/g" $(REGRESSION_DIR)/version_cli.expect > $(REGRESSION_DIR)/version_cli.expect.output; \
	./$(BIN) --version > $(REGRESSION_DIR)/version_cli.output; \
	diff -u $(REGRESSION_DIR)/version_cli.expect.output $(REGRESSION_DIR)/version_cli.output; \
	n=$$((n + 1)); \
	echo "snapshot tests passed: $$n/$$total"

test-run: test
	$(MAKE) snapshot-run

# gameplay .c files must not call printf (use grendr render_* instead)
check-layers:
	@violators=$$(grep -l 'printf' src/*.c 2>/dev/null | grep -vE '/(main|grendr|platdos|platpos|platwin)\.c$$' || true); \
	if [ -n "$$violators" ]; then \
		echo "layer violation: printf only allowed in main.c, grendr.c, platdos.c, platpos.c, platwin.c"; \
		echo "$$violators"; \
		exit 1; \
	fi

# Unit tests (greatest, TEST_MODE, not linked into release dosmud)
UNIT_BUILD_DIR = tests/unit/build
UNIT_COVERAGE_DIR = $(UNIT_BUILD_DIR)/coverage
UNIT_COV_BUILD_DIR = $(UNIT_BUILD_DIR)/coverage-build
UNIT_BIN = $(UNIT_BUILD_DIR)/dosmud_unit
UNIT_COV_BIN = $(UNIT_COV_BUILD_DIR)/dosmud_unit_cov
UNIT_CFLAGS = $(TEST_CFLAGS) -I$(UNIT_DIR)
UNIT_COV_CFLAGS = $(UNIT_CFLAGS) -fprofile-arcs -ftest-coverage
UNIT_GAMEPLAY_SRC = $(PLAT_SRC) src/buildid.c src/game.c src/gout.c src/gprog.c src/gstory.c src/gwhok.c src/combat.c src/genc.c \
	src/dialogue.c src/npc.c src/gatmos.c src/grendr.c src/fmt.c src/invent.c \
	src/command.c src/world.c src/items.c src/replay.c src/save.c src/txtres.c
UNIT_CORE_SRC = $(UNIT_GAMEPLAY_SRC) $(HARNESS_SRC)
UNIT_TEST_SRC = $(UNIT_DIR)/unit_main.c $(UNIT_DIR)/unit_util.c $(UNIT_DIR)/unit_item.c \
	$(UNIT_DIR)/unit_gout.c \
	$(UNIT_DIR)/unit_rplog.c \
	$(UNIT_DIR)/unit_save.c \
	$(UNIT_DIR)/unit_cmd.c $(UNIT_DIR)/unit_harn.c $(UNIT_DIR)/unit_inv.c $(UNIT_DIR)/unit_cbt.c \
	$(UNIT_DIR)/unit_gprog.c $(UNIT_DIR)/unit_gstry.c $(UNIT_DIR)/unit_gwhok.c $(UNIT_DIR)/unit_genc.c $(UNIT_DIR)/unit_dial.c $(UNIT_DIR)/unit_npc.c \
	$(UNIT_DIR)/unit_gatmos.c $(UNIT_DIR)/unit_fmt.c $(UNIT_DIR)/unit_wrld.c $(UNIT_DIR)/unit_game.c $(UNIT_DIR)/unit_tharn.c
UNIT_CORE_OBJS = $(addprefix $(UNIT_BUILD_DIR)/,$(notdir $(UNIT_CORE_SRC:.c=.o)))
UNIT_TEST_OBJS = $(addprefix $(UNIT_BUILD_DIR)/,$(notdir $(UNIT_TEST_SRC:.c=.o)))
UNIT_COV_CORE_OBJS = $(addprefix $(UNIT_COV_BUILD_DIR)/,$(notdir $(UNIT_CORE_SRC:.c=.o)))
UNIT_COV_TEST_OBJS = $(addprefix $(UNIT_COV_BUILD_DIR)/,$(notdir $(UNIT_TEST_SRC:.c=.o)))
COVERAGE_MODULES = command invent combat game genc dialogue npc gatmos world gprog gstory gwhok items fmt gout replay save testharn

ifeq ($(UNIT_BUILD_VERBOSE),1)
UNIT_CC_QUIET =
UNIT_LINK_ANNOUNCE =
else
UNIT_CC_QUIET = @
UNIT_LINK_ANNOUNCE = @echo "building $(UNIT_BIN)..."
endif

$(UNIT_BUILD_DIR)/%.o: src/%.c $(VERSION_HDR)
	@mkdir -p $(UNIT_BUILD_DIR)
	$(UNIT_CC_QUIET)$(CC) $(UNIT_CFLAGS) -c $< -o $@

$(UNIT_BUILD_DIR)/%.o: $(UNIT_DIR)/%.c $(VERSION_HDR)
	@mkdir -p $(UNIT_BUILD_DIR)
	$(UNIT_CC_QUIET)$(CC) $(UNIT_CFLAGS) -c $< -o $@

$(UNIT_BUILD_DIR)/%.o: $(HARNESS_DIR)/%.c $(VERSION_HDR)
	@mkdir -p $(UNIT_BUILD_DIR)
	$(UNIT_CC_QUIET)$(CC) $(UNIT_CFLAGS) -c $< -o $@

$(UNIT_COV_BUILD_DIR)/%.o: src/%.c $(VERSION_HDR)
	@mkdir -p $(UNIT_COV_BUILD_DIR)
	$(UNIT_CC_QUIET)$(CC) $(UNIT_COV_CFLAGS) -c $< -o $@

$(UNIT_COV_BUILD_DIR)/%.o: $(UNIT_DIR)/%.c $(VERSION_HDR)
	@mkdir -p $(UNIT_COV_BUILD_DIR)
	$(UNIT_CC_QUIET)$(CC) $(UNIT_COV_CFLAGS) -c $< -o $@

$(UNIT_COV_BUILD_DIR)/%.o: $(HARNESS_DIR)/%.c $(VERSION_HDR)
	@mkdir -p $(UNIT_COV_BUILD_DIR)
	$(UNIT_CC_QUIET)$(CC) $(UNIT_COV_CFLAGS) -c $< -o $@

$(UNIT_BIN): $(UNIT_CORE_OBJS) $(UNIT_TEST_OBJS)
	@mkdir -p $(UNIT_BUILD_DIR)
	$(UNIT_LINK_ANNOUNCE)
	$(UNIT_CC_QUIET)$(CC) $(UNIT_CFLAGS) -o $@ $(UNIT_CORE_OBJS) $(UNIT_TEST_OBJS)

$(UNIT_COV_BIN): $(UNIT_COV_CORE_OBJS) $(UNIT_COV_TEST_OBJS)
	@mkdir -p $(UNIT_COV_BUILD_DIR)
	$(UNIT_LINK_ANNOUNCE)
	$(UNIT_CC_QUIET)$(CC) $(UNIT_COV_CFLAGS) -o $@ $(UNIT_COV_CORE_OBJS) $(UNIT_COV_TEST_OBJS)

build-unit:
	$(call RUN_TIMED,$(MAKE) $(UNIT_BIN))

test-unit: $(UNIT_BIN)
	./$(UNIT_BIN)

test-unit-verbose: $(UNIT_BIN)
	./$(UNIT_BIN) --verbose

test-unit-verbose-gameplay: $(UNIT_BIN)
	./$(UNIT_BIN) --verbose-gameplay

test-unit-coverage: $(UNIT_COV_BIN)
	@rm -f $(UNIT_COV_BUILD_DIR)/*.gcda
	./$(UNIT_COV_BIN)
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
		stats=$$(cd $(UNIT_COVERAGE_DIR) && gcov -b -o $(CURDIR)/$(UNIT_COV_BUILD_DIR) $$covsrc 2>/dev/null | awk ' \
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
	$(MAKE) UNIT_BUILD_VERBOSE=1 $(UNIT_COV_BIN)
	@rm -f $(UNIT_COV_BUILD_DIR)/*.gcda
	./$(UNIT_COV_BIN)
	@mkdir -p $(UNIT_COVERAGE_DIR)
	@for f in $(COVERAGE_MODULES); do \
		echo "=== $$f ==="; \
		if [ "$$f" = "testharn" ]; then \
			covsrc=$(CURDIR)/$(HARNESS_DIR)/testharn.c; \
		else \
			covsrc=$(CURDIR)/src/$$f.c; \
		fi; \
		(cd $(UNIT_COVERAGE_DIR) && gcov -b -o $(CURDIR)/$(UNIT_COV_BUILD_DIR) $$covsrc 2>/dev/null | grep -E '^File|^Lines|^Branches') || true; \
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

$(SOAK_BUILD_DIR)/%.o: src/%.c $(VERSION_HDR)
	@mkdir -p $(SOAK_BUILD_DIR)
	@$(CC) $(SOAK_CFLAGS) -c $< -o $@

$(SOAK_BUILD_DIR)/%.o: $(SOAK_DIR)/%.c $(VERSION_HDR)
	@mkdir -p $(SOAK_BUILD_DIR)
	@$(CC) $(SOAK_CFLAGS) -c $< -o $@

$(SOAK_BUILD_DIR)/%.o: $(HARNESS_DIR)/%.c $(VERSION_HDR)
	@mkdir -p $(SOAK_BUILD_DIR)
	@$(CC) $(SOAK_CFLAGS) -c $< -o $@

$(SOAK_BUILD_DIR)/unit_util.o: $(UNIT_DIR)/unit_util.c $(VERSION_HDR)
	@mkdir -p $(SOAK_BUILD_DIR)
	@$(CC) $(SOAK_CFLAGS) -c $< -o $@

$(SOAK_BIN): $(SOAK_CORE_OBJS) $(SOAK_TEST_OBJS)
	@mkdir -p $(SOAK_BUILD_DIR)
	@echo "building $(SOAK_BIN)..."
	@$(CC) $(SOAK_CFLAGS) -o $@ $(SOAK_CORE_OBJS) $(SOAK_TEST_OBJS)

build-soak:
	$(call RUN_TIMED,$(MAKE) $(SOAK_BIN))

test-soak: $(SOAK_BIN)
	./$(SOAK_BIN)

clean:
	rm -f $(BIN)
	rm -f dosmud.exe
	rm -rf $(UNIT_BUILD_DIR)
	rm -rf $(BUILD_INCLUDE_DIR)
	rm -rf $(SOAK_BUILD_DIR)
	rm -f $(REGRESSION_DIR)/*.output tests/*.output
	rm -f dosmud_unit dosmud_unit-*.gcno dosmud_unit-*.gcda *.gcov src/*.gcno src/*.gcda

dos-prepare:
	powershell.exe -ExecutionPolicy Bypass -File dos-prepare.ps1 $(if $(MODE),-Mode $(MODE)) $(if $(NORUN),-NoRun) $(if $(SEED),-Seed $(SEED))

test-dos-prepare:
	$(MAKE) dos-prepare MODE=$(TEST_MODE_FLAG) $(if $(NORUN),NORUN=1) $(if $(SEED),SEED=$(SEED))

dos-prepare-norun:
	$(MAKE) dos-prepare NORUN=1 $(if $(SEED),SEED=$(SEED))

test-dos-prepare-norun:
	$(MAKE) test-dos-prepare NORUN=1 $(if $(SEED),SEED=$(SEED))

dos-run:
	powershell.exe -ExecutionPolicy Bypass -File dos-prepare.ps1 -NoBuild $(if $(SEED),-Seed $(SEED))

.PHONY: FORCE build-all build build-win win-run run test-all test test-win test-run-bin snapshot-run test-run build-unit test-unit test-unit-verbose test-unit-verbose-gameplay test-unit-coverage test-unit-coverage-verbose build-soak test-soak check-layers clean dos-prepare test-dos-prepare dos-prepare-norun test-dos-prepare-norun dos-run
