# Testing and Build Validation

This page is the canonical source for build/test command workflow.

## Native local checks

Run from project root:

```sh
make build
make check-layers
make test
make test-run
make test-unit
make test-soak
```

Purpose:

- `make build`: native GCC development build; prints `elapsed: <seconds>` after the compile/link step
- `make build-win`: WSL cross-compile to a native Windows console `dosmud.exe` with `x86_64-w64-mingw32-gcc`
- `make win-run`: launches the existing Windows `dosmud.exe` in a new Windows console window; does not build
- `make check-layers`: core/render boundary guard (no `printf` in `src/*.c` except `main.c`, `grendr.c`, and the platform files `platpos.c`, `platwin.c`, and `platdos.c`)
- `make test`: strict deterministic compile (`-Werror`, `-DTEST_MODE`, `-g -O0`); does not run `check-layers`; prints `elapsed: <seconds>` after the compile/link step
- `make test-win`: WSL cross-compile of the native Windows console `TEST_MODE` executable (`dosmud.exe`); compile-only, no snapshot run from Linux
- `make snapshot-run`: runs every name in `SNAPSHOT_TESTS` plus `seed_cli` against the existing native `TEST_MODE` binary (`./dosmud`; see [Snapshot test files](#snapshot-test-files)). Each step prints `snapshot: <name>`. Finishes with `snapshot tests passed: N/M` (for example `66/66` snapshots plus `seed_cli`, 67 steps total).
- `make test-run`: builds the test binary (`make test`), then runs `make snapshot-run`.
- `make test-unit`: builds and runs the greatest unit suite (`tests/unit/build/dosmud_unit`, `TEST_MODE` only; not linked into release `dosmud`)
- `make test-soak`: builds and runs long-run soak/stress checks (`tests/soak/build/dosmud_soak`; separate from unit tests)

## Interactive helpers

Use these when you want to launch a playable or interactive binary rather than run a validation step:

- `make run`: builds the native release binary if needed, then launches it; pass `SEED=<unsigned>` to forward `--seed`
- `make test-run-bin`: builds the native `TEST_MODE` binary if needed, then launches it; pass `SEED=<unsigned>` to forward `--seed`
- `make dos-run`: launches the existing prepared DOS release executable without rebuilding
- `make win-run`: launches the existing repo-root Windows `dosmud.exe` from the most recent `make build-win` or `make test-win`; pass `SEED=<unsigned>` to forward `--seed`

For the WSL -> Windows path, build `dosmud.exe` with `make build-win` or `make test-win`, then launch it with `make win-run` or directly from Windows PowerShell, `cmd.exe`, or Windows Terminal. `win-run` launches whatever repo-root `dosmud.exe` was produced by the most recent Windows cross-build, opens a new Windows console window, and forwards `SEED=<n>` when set. This issue adds a console app path only; a GUI or alternate renderer remains separate work.

Native runs may also pass `--replay-log <path>` directly to `./dosmud` or `./dosmud --seed <n>`. The replay log is a sidecar text file, so it does not change snapshot stdout output by itself.

## Test layers

| Layer | Command | What it proves |
|-------|---------|----------------|
| Snapshots | `make test-run` or `make snapshot-run` | Player-visible output matches golden `.expect` files |
| Unit tests | `make test-unit` | Small, targeted `GameState` / API behavior |
| Soak tests | `make test-soak` | Fixed-seed long runs: state stays legal, perf within ceilings |

`make test-all` runs check-layers, snapshots, unit coverage, then soak.

## When to add or update tests

Use this section when deciding what to write, not only what to run. Agents and contributors should follow it before opening a PR. Pre-PR runs: [`.cursor/rules/testing-discipline.mdc`](../.cursor/rules/testing-discipline.mdc) and [AGENTS.md](../AGENTS.md#testing-expectations).

| Change | Snapshots | Unit (`tests/unit/unit_*.c`) |
|--------|-----------|------------------------------|
| New verb or changed player-visible output | Yes | Yes |
| New `*_cmd_*` or other exported handler in a slice | If output changes | **Yes - call the API directly** |
| Behavior-preserving refactor into another `.c` | Only if `.expect` files drift | Update the slice suite when APIs are new |
| `game.c` static router only | No | Add or extend `unit_game.c` / `game_process_input` tests **when** router or tick semantics change; otherwise no new unit file |
| `world_init` or fixed graph layout | If travel or map output changes | Update [`tests/harness/th_world.c`](../tests/harness/th_world.c) (and fixtures if needed) |

**Unit file convention:** add or extend `tests/unit/unit_<basename>.c` for the gameplay module you changed (see `UNIT_TEST_SRC` in the `Makefile`). Suites use abbreviated basenames, not full module names - for example `command.c` → `unit_cmd.c`, `invent.c` → `unit_inv.c`, `combat.c` → `unit_cbt.c`. FAT 8.3 truncations also apply (`dialogue.c` → `unit_dial.c`, `world.c` → `unit_wrld.c`). Which modules need coverage is listed under [In-scope modules](#unit-tests-greatest) below - do not maintain a second module table here.

**Lesson from [#90](https://github.com/ianmays/dosmud/issues/90):** when command handling moves from `game.c` into a slice module, add tests in that slice's `unit_*.c` file. Green `unit_game.c` tests that only call `game_process_input` do not document slice ownership or catch regressions in new entry points.

**For `GameEvent` producer work:** assert payload contracts where the producer owns them. Keep queue reset/overflow/order rules in `unit_gout.c`, router sequencing in `unit_game.c`, slice payload semantics in the owning `unit_*.c`, and harness fixture shape in `unit_harn.c`. Snapshots should prove rendered text, not replace direct payload assertions.

## Test gap audit (agents and CI)

Before a draft PR, agents run a **test-gap pass** ([`.cursor/skills/testing-gap-auditor/SKILL.md`](../.cursor/skills/testing-gap-auditor/SKILL.md), [`.cursor/rules/testing-gap-after-implement.mdc`](../.cursor/rules/testing-gap-after-implement.mdc)):

```sh
git fetch origin main
sh scripts/check-test-gaps.sh origin/main
```

Pull requests to `main` run the same script in CI in **informative** mode (`TEST_GAP_INFORMATIVE=1`, `continue-on-error`): gaps are logged but do not fail the job. Treat reported gaps as merge blockers until fixed or the heuristics are tuned. The script reads `COVERAGE_MODULES`, `UNIT_GAMEPLAY_SRC`, `PLAT_SRC`, and `HARNESS_SRC` from the `Makefile`, resolves owning unit suites from [`tests/unit/module-map`](../tests/unit/module-map), and flags likely missing unit or snapshot updates. Unit gaps are not waived by unrelated snapshot-only changes on the same branch.

**Local:** run without `TEST_GAP_INFORMATIVE` before a draft PR (exit 1 on gaps). **`TEST_GAP_WAIVE=1`** skips the script locally for debugging only; CI does not set it. `make test-unit-coverage` remains the branch-coverage bar; the gap script does not replace it.

## Test layout

```text
tests/
  harness/        # testharn.c (@fixture DSL), th_world.c (seed-1234 graph); not in src/
  regression/     # snapshot golden files only: <name>.input, <name>.expect
  unit/           # greatest harness: greatest.h, unit_*.c, unit_util.*
    build/        # generated: dosmud_unit, *.o, *.gcno, *.gcda (gitignored)
      coverage/   # *.gcov reports from make test-unit-coverage (gitignored)
  soak/           # stress harness: soak_*.c (links unit_util + th_world)
    build/        # generated: dosmud_soak (gitignored)
```

Snapshot regression lives under [`tests/regression/`](../tests/regression/) (data only). Fixture code lives under [`tests/harness/`](../tests/harness/). Unit sources live under [`tests/unit/`](../tests/unit/). Soak sources live under [`tests/soak/`](../tests/soak/). The unit binary and coverage profiles are written to [`tests/unit/build/`](../tests/unit/build/) (ignored by git). Release `dosmud` and snapshot `*.output` are also ignored via [`.gitignore`](../.gitignore).

[`tests/harness/testharn.c`](../tests/harness/testharn.c) implements the `@fixture` / `@seed` DSL used by snapshot `.input` files and by unit tests in [`tests/unit/unit_tharn.c`](../tests/unit/unit_tharn.c). It is not snapshot-only.

## Unit tests (greatest)

Unit test coverage ([#95](https://github.com/ianmays/dosmud/issues/95)) uses [greatest](https://github.com/silentbicycle/greatest) **1.5.0** vendored as [`tests/unit/greatest.h`](../tests/unit/greatest.h). Upstream license stays in the header; a small dosmud patch adds quiet-by-default output (`greatest_set_quiet`, `GREATEST_FLAG_QUIET`) in a separate git commit from the unmodified vendor import.

```sh
make test-unit
make build-unit                      # build only; useful when timing compile cost
make test-unit-verbose              # greatest suite/test progress only
make test-unit-verbose-gameplay     # greatest progress + gameplay render text
make test-unit-coverage             # runs test-unit, then compact branch/line % table
make test-unit-coverage-verbose     # same tests, full gcov block per module
```

- Binary: `tests/unit/build/dosmud_unit` (all gameplay modules except `main.c`, plus `tests/unit/unit_*.c`)
- `make build-unit` prints `elapsed: <seconds>` after the unit binary build finishes
- Output levels:

| Target / flag | Greatest | Gameplay `render_*` |
|---------------|----------|---------------------|
| `make test-unit` (default) | final summary only (`FAIL` lines on error) | suppressed |
| `make test-unit-verbose` (`--verbose`, `-v`) | suite headers, per-test `PASS`/`FAIL`, per-suite summary | suppressed |
| `make test-unit-verbose-gameplay` (`--verbose-gameplay`) | same as verbose | enabled |

**Coverage report levels:**

| Target | Output |
|--------|--------|
| `make test-unit-coverage` | Quiet `test-unit`, then one line per module (`branch % / line %`), a `-------` separator, weighted `overall` row, and `below 90% branch: ...` if any module misses the bar |
| `make test-unit-coverage-verbose` | `test-unit` with compile lines echoed, then `=== module ===` blocks with full `gcov` lines (for debugging gaps) |

`make test-all` uses quiet `test-unit-coverage`. Run `make clean` before coverage if stale `.gcda` files produce libgcov checksum warnings.

- Determinism: call `plat_seed_rng(fixed_seed)` in setup; use `game_roll_inject_*` and `CFG_TEST_*` for asserted combat/intimidate outcomes
- Snapshots assert player-visible output; unit tests assert `GameState` and parse results

**In-scope modules (branch coverage target ~90%+):** `command`, `invent`, `combat`, `game`, `genc`, `wanderer`, `dialogue`, `gatmos`, `world`, `gprog`, `items`, `fmt`, `gout`, `testharn`

**Out of scope for the unit coverage bar:** `grendr`, `txtres`, `main`, `platpos` / `platwin` / `platdos` (presentation glue; `fmt` holds testable format logic; snapshots cover printed output and ASCII art)

**Harness-only fixture:** `bag_full_gate` - applies `game_inv_bag_add` without resetting baseline; returns fixture failure (`-2`) when the bag is already full (used by unit tests for `testharn_apply` error paths)

## Soak / stress tests ([#116](https://github.com/ianmays/dosmud/issues/116))

Separate binary from unit tests: `tests/soak/build/dosmud_soak` via `make test-soak`. Uses the same greatest runner and linked game modules as unit tests, but runs long fixed-seed loops and checks that `GameState` stays legal (HP, mode, room, bag, dialogue/combat fields).

```sh
make build-soak
make test-soak
```

`make build-soak` prints `elapsed: <seconds>` after the soak binary build finishes.

Scenarios (see [`tests/soak/soak_sim.c`](../tests/soak/soak_sim.c)):

| Test | Loop |
|------|------|
| `soak_background_ticks` | `CFG_TEST_SOAK_TICKS` (10000) × `game_background_step` with full wanderer/bandit/atmosphere |
| `soak_command_wait_move` | 10000 × alternate `wait` / `move north` with `test_quiet_ticks` |
| `soak_combat_loop` | `CFG_TEST_SOAK_COMBAT_ROUNDS` (200) × one-hit combat victory with roll inject |

Each scenario prints a machine-readable benchmark line (limits come from `CFG_TEST_SOAK_LIMIT_*` in [`include/config.h`](../include/config.h)):

```text
SOAK_BENCH <name> ticks=<n> us_per_tick=<u> limit=<L>
```

The test fails if `us_per_tick` exceeds `limit`. CI ([`scripts/ci-stats.sh`](../scripts/ci-stats.sh)) runs `make test-soak` as its own step and parses `limit=` from the log for the PR **Soak benchmarks** table (no separate limits file).

After intentional performance changes, run `make test-soak` locally and raise the matching `CFG_TEST_SOAK_LIMIT_*` macros only when the new baseline is expected (~2× measured on the CI runner is a reasonable starting margin). Coarse `clock()` resolution may report `us_per_tick=0` on fast runs; limits are regression guards, not micro-benchmarks.

World boot uses [`tests/harness/th_world.c`](../tests/harness/th_world.c) via [`tests/unit/unit_util.c`](../tests/unit/unit_util.c) (`unit_game_fresh` / `harness_world_boot_graph`). When `world_init` changes, update **one** graph in `th_world.c`.

## Test fixtures (`TEST_MODE` only)

Snapshot tests can set up known game state without walking RNG-dependent commands. In a `make test` binary, harness lines in `.input` files of the form:

```text
@fixture <name>
@seed <unsigned>
```

are handled by `testharn` before normal command parsing. Harness lines are not echoed as player commands. Unknown fixture names print `unknown test fixture` to stderr and exit with status 1. When a known fixture cannot finish setup, the binary prints `test fixture failed` to stderr and also exits with status 1. Invalid `@seed` syntax prints `invalid @seed` to stderr and exits with status 1.

Prefer fixtures over long setup scripts when a test needs a specific mode, inventory, or encounter. After changing fixture output, regenerate the matching `.expect` with `make test-run` and review the diff.

Fixtures call `game_reset_fixture_baseline` first (same mutable fields as `game_init`: mode, room, tick, player stats, bag, combat, wanderer, corpses, ground items, env focus, and map exploration). That leaves the world graph and `GameState.seed` unchanged. `main.c` then calls `plat_seed_rng(game.seed)` so libc `rand()` matches that seed and follow-up rolls do not depend on earlier commands in the same run.

**Bandit / combat** (camp baseline; bandit setups clear camp ground items so the stick is only in bag or wield slot):

| Fixture | State |
|---------|--------|
| `bandit_dialogue` | Base reset, stick in bag, bandit dialogue open |
| `bandit_handover_pick` | Base reset, stick in bag, bandit dialogue open, handover pick prompt (reply 2 already chosen) |
| `bandit_wielded_pick` | Base reset, stick wielded (`Atk:1`), bandit dialogue open, handover pick prompt |
| `bandit_combat_turn1` | Base reset, stick wielded, combat mode, player HP 20, bandit HP at `CFG_COMBAT_ENEMY_HP_BASE` (combat start only) |
| `bandit_combat_turn1_resolve` | Same as `bandit_combat_turn1`, then injected roll queue + `combat_resolve_reply(1)` for `equipment` (see trade-offs below) |
| `bandit_dialogue_empty` | Bandit dialogue, empty bag, no wielded weapon |
| `bandit_combat_defend_ready` | Combat turn 1 + inject queue for enemy damage after defend |
| `bandit_combat_salve_ready` | Combat turn 1, salve in bag + inject for enemy turn after salve (player at max HP via `fixture_bandit_combat_turn1`) |
| `bandit_combat_level_ready` | Near-kill combat + inject for victory + `xp = 19` (level-up test) |
| `bandit_fight_ready` | Bandit dialogue + inject for `combat_start` enemy HP spread |
| `bandit_intimidate_ok` | Bandit dialogue + inject (`CFG_TEST_INTIMIDATE_OK`) for reply `3` success |
| `bandit_intimidate_fail` | Bandit dialogue + inject (`CFG_TEST_INTIMIDATE_FAIL`) for reply `3` failure |
| `bandit_victory_spear` / `stick` / `berry` / `herb` / `fish` | Near-kill + inject hit, corpse loot percent, kill XP |

**Exploration / world** (room and map state without bandit dialogue):

| Fixture | State |
|---------|--------|
| `world_boot` | Replace `World` graph via `world_apply_graph` with harness-owned seed-1234 tables; does not reset player state |
| `world_linear` | Same graph as `world_boot` (alias until a slimmer preset exists) |
| `at_camp` | Camp, tick 0, explore, camp explored on map |
| `at_road` | Road, tick 1, explore, camp and road explored on map |
| `at_marsh_reed` | Marsh, tick 2, stick in bag, reed on ground, camp and marsh explored |
| `quiet_camp_dual_ground` | Camp, quiet ticks on, stick and reed on ground for multi-item pickup tests |
| `quiet_camp_dual_ground_full_bag` | `quiet_camp_dual_ground` plus a full bag for take-all refusal snapshots |
| `at_pond` / `at_tower` / `at_orchard` / `at_catacombs` | Named room, explore, room explored |
| `quiet_explore` | `at_camp` + `test_quiet_ticks` + wanderer off (for `wait` / `move` snapshots) |

**Wanderer** (co-located dialogue without tick movement):

| Fixture | State |
|---------|--------|
| `wanderer_dialogue` | Road, tick 0, player and wanderer co-located, wanderer dialogue open (intro + options rendered) |

**Bags / items** (explore, named room):

| Fixture | State |
|---------|--------|
| `bag_berry` / `bag_fish` / `bag_salve` / `bag_torch` / `bag_spear` / `bag_stone` / `bag_stick` | Camp or pond baseline + one item in bag |
| `bag_stacked` | Camp + two berries and one stick in bag (stacked bag display) |
| `bag_berry_low_hp` / `bag_fish_low_hp` | Same as `bag_berry` / `bag_fish` with `player_hp = CFG_START_MAX_HP - 5` |
| `bag_craft_salve` | Camp + herb and berry in bag |

**Inspect focus** (camp):

| Fixture | State |
|---------|--------|
| `env_focus_rustle` / `creak` / `water` / `grit` | Active focus of that kind, valid `expires_tick` |

**Loot helpers** (camp):

| Fixture | State |
|---------|--------|
| `corpse_stripped` | Corpse present, no loot item |
| `corpse_loot_full_bag` | Full bag + corpse with stick loot |

For marsh item/craft snapshots, prefer `at_marsh_reed` over walking camp intimidate plus `south` (avoids tick RNG on travel). For movement that depends on exit layout (`north` from marsh, `move north` from camp), chain `@fixture world_boot` before room fixtures so the graph stays stable if `world_init` changes.

```text
@fixture world_boot
@fixture at_marsh_reed
```

### Quiet ticks (`test_quiet_ticks`, `TEST_MODE` only)

`quiet_explore` sets `GameState.test_quiet_ticks` and disables the wanderer. While set, `advance_world_tick` only increments the tick and runs `world_step`; it skips animal noise, atmosphere, bandit ambush rolls, and wanderer movement/spawn. Use this for snapshots that call `wait` or `move` so output does not depend on ambient libc `rand()`.

### `@seed` in `.input` files

`@seed <unsigned>` sets `GameState.seed` mid-file and reseeds libc RNG (same rules as CLI `--seed`: decimal, non-negative, at most `CFG_SEED_CLI_MAX`; no leading `+`/`-`). After each successful `@seed` or `@fixture`, `main.c` calls `plat_seed_rng(game.seed)` so later rolls do not depend on earlier commands in the same run. CLI `--seed` still applies only at process start (`tests/regression/seed_cli.*`).

Use `@seed` when a snapshot block needs a different libc RNG stream without starting a new process. Add it to a snapshot only when that isolation is required; do not rely on seed alone for asserted mechanics; use inject or teleport.

### Fixture design trade-offs

Snapshots use three determinism levels:

1. **Teleport state** - fixtures set `GameState` directly. Do not walk RNG-heavy setup (`take stick`, intimidate, random bandit spawn). `tests/regression/map.*` checks map **render** with explored flags set by the fixture; `tests/regression/walk_map.*` checks `room_explored` updated by a real `move` (with `quiet_explore`).

2. **Inject rolls** (`TEST_MODE` only) - use `game_roll_inject_begin` for any asserted outcome that goes through `game_roll_spread` / `game_roll_percent`: combat damage, corpse loot tier, kill XP, bandit intimidate (reply `3`), and `combat_start` enemy HP. Constants live under `#ifdef TEST_MODE` in [`config.h`](../include/config.h) (`CFG_TEST_EQUIPMENT_*`, `CFG_TEST_COMBAT_*`, `CFG_TEST_INTIMIDATE_*`, `CFG_TEST_VICTORY_*`). Do not bypass combat with render-only hit lines. If combat tuning changes, update those constants and `.expect`.

3. **Seed reset** - after each `@fixture` or `@seed`, `main.c` calls `plat_seed_rng(game.seed)` for stream isolation between harness blocks. Do not rely on seed alone for asserted mechanics; use inject or teleport.

4. **Quiet ticks** - for tick-advancing commands in explore mode, use `quiet_explore` (see above).

5. **World graph** (`TEST_MODE` only) - `@fixture world_boot` calls `harness_world_boot_graph` in [`tests/harness/th_world.c`](../tests/harness/th_world.c) (seed-1234 layout). Unlike rolls, the graph is **not** cleared by `game_reset_fixture_baseline`; it persists until the next `world_boot` or a new process. Unit and soak tests use the same graph through `unit_world_boot_graph` in [`tests/unit/unit_util.c`](../tests/unit/unit_util.c). When `world_init` changes, refresh **only** `th_world.c`.

Bandit intimidate in gameplay uses `game_roll_percent` (not raw `rand()`), so intimidate snapshots stay on the inject path.

Add new fixtures in [`tests/harness/testharn.c`](../tests/harness/testharn.c) and document them here. `testharn` + `th_world` are linked for `make test` / `dos-prepare MODE=TEST_MODE` and `make test-unit`, not for `make build` or `make test-soak` (soak links `th_world` only).

| Fixture | Notes |
|---------|--------|
| `bag_full_gate` | No baseline reset; fails setup when `game_inv_bag_add` cannot store `ITEM_STICK` (bag already full) |

### Adding a snapshot test

1. Add `tests/regression/<name>.input` (and prefer one scenario per file).
2. Use `@fixture` for setup; add inject in the fixture or via a `*_ready` fixture when outcomes must be fixed.
3. Use `quiet_explore` when the test calls `wait` or `move`.
4. Run `make test && make snapshot-run` (or `./dosmud < tests/regression/<name>.input > tests/regression/<name>.output`) and copy or diff against `tests/regression/<name>.expect`.
5. Add `<name>` to `SNAPSHOT_TESTS` in the [Makefile](../Makefile) (`seed_cli` stays separate: it uses `--seed` with `smoke.input`).
6. Document new fixtures in this file.

### Snapshot test files

Each process run uses one `.input` file until `quit`. `make snapshot-run` runs `SNAPSHOT_TESTS` (includes `smoke`), then `seed_cli`. `make test-run` is the compile-plus-run wrapper.

**Core / inventory (also in `SNAPSHOT_TESTS`):** `smoke`, `bandit_handover`, `bandit_wielded_give`, `area_items`, `map`, `equipment`, `craft_wielded`, `take_all`, `take_all_bag_full`, `bag_view`, `bag_stacks`.

**Movement / time:** `walk_north`, `walk_map`, `wait_tick`.

**NPC talk:** `frog_replies`, `watchman_talk`, `wanderer_replies`, `wanderer_talk_blocked`, `herbalist_talk`, `archivist_talk`, `talk_nobody`, `game_event_dialogue` (pond frog, bandit camp talk, and tower watchman through generic `GAME_EVENT_DIALOGUE` / `GAME_EVENT_ENCOUNTER` paths; [#175](https://github.com/ianmays/dosmud/pull/175)).

**Eat / use:** `use_salve`, `use_torch`, `use_spear`, `use_stone`, `eat_berry`, `eat_fish`, `eat_berry_heal`, `eat_fish_heal`, `eat_not_edible`, `eat_missing`.

**Inspect:** `inspect_rustle`, `inspect_creak`, `inspect_water`, `inspect_grit`, `inspect_none`, `inspect_wrong`.

**Ambient (tick-driven):** `ambient_rustle`, `ambient_tick_order`, `ambient_item` (`@fixture ambient_camp` plus `@seed` for deterministic atmosphere, animal noise, and nearby-item paths).

**Combat:** `combat_defend`, `combat_salve`, `combat_no_salve`, `combat_invalid`, `combat_take_blocked`, `combat_victory_xp`, `level_up`.

**Loot:** `loot_spear`, `loot_stick`, `loot_berry`, `loot_herb`, `loot_fish`, `loot_empty`, `loot_stripped`, `loot_bag_full`.

**Bandit dialogue:** `bandit_fight`, `bandit_intimidate_ok`, `bandit_intimidate_fail`, `bandit_bag_empty`.

**Meta / inventory:** `unknown_cmd`, `cannot_move`, `give_wrong_context`, `reply_nobody`, `post_combat_reply_guard`, `reply_invalid`, `craft_salve`, `craft_unknown`, `take_nothing`, `take_wrong_item`.

## DOS/Open Watcom validation path

Use PowerShell-driven DOS prep from Linux host shell to build and sync the DOS tree:

```sh
make dos-prepare
```

TEST_MODE DOS prep without spelling the mode flag directly:

```sh
make test-dos-prepare
```

Build and validate the DOS tree without launching the runtime session:

```sh
make dos-prepare-norun
make test-dos-prepare-norun
```

Start DOS and launch the most recently prepared DOS executable without rebuilding or refreshing the tree:

```sh
make dos-run
```

There is one prepared DOS tree at `$destination`. `make dos-run` launches whatever executable was most recently prepared there. Run `make dos-prepare` for release mode or `make test-dos-prepare` for `TEST_MODE` before using `make dos-run`.

`make dos-prepare` now prints `elapsed build.bat time: <seconds>` after the Open Watcom build finishes and before the runtime DOS session starts. That elapsed time measures `build.bat` only, not the PowerShell tree refresh/copy phase. When `build.log` is present in the prepared DOS tree, `dos-prepare.ps1` appends the same elapsed line there too.

To pass a custom seed through the DOS helper targets, use `SEED=<unsigned>`, for example:

```sh
make dos-prepare SEED=1234
make test-dos-prepare SEED=1234
make dos-prepare-norun SEED=1234
make test-dos-prepare-norun SEED=1234
make dos-run SEED=1234
```

When you add or remove `src\*.c` files, update `Makefile` (`SRC` or `TEST_SRC`) and `build.bat`. For the Open Watcom path, keep every `wcl` and `wlib` line under the COMMAND.COM length limit (about 127 characters): gameplay sources are packed into `gameplay.lib` via several short `wlib` calls; the final `wcl` link lists `main.obj`, `platdos.obj`, `gameplay.lib`, plus the other `.obj` files. `TEST_MODE` copies [`tests/harness/`](../tests/harness/) to `harness\` via `dos-prepare.ps1`, compiles `th_world.c` / `testharn.c` to `thwld.obj` / `tharn.obj`, and archives both into `gameplay.lib`. Use `goto` labels in `build.bat` for conditionals; parenthesized `if (...)` blocks break under COMMAND.COM.

Deterministic DOS validation:

```sh
make dos-prepare MODE=TEST_MODE
```

Runtime seed (native or DOS build): the startup banner always prints the active seed, for example `dosmud (seed 1234)`. In `TEST_MODE` the default is `CFG_TEST_RAND_SEED` unless overridden on the command line:

```sh
./dosmud --seed 1234
make run SEED=1234
make test-run-bin SEED=1234
make win-run SEED=1234
make dos-prepare SEED=1234
make test-dos-prepare SEED=1234
```

Invalid flags print `usage: dosmud [--seed <unsigned>]` to stderr and exit with status 1. Seed values must be decimal, non-negative, and at most `CFG_SEED_CLI_MAX` (4294967295); leading `+`/`-` and out-of-range values are rejected.

## Combined cross-path checks

When changing build flow/tooling or other high-risk runtime behavior:

```sh
make build-all
make test-all
```

These targets intentionally exercise DOS build prep and native GCC flow together. They validate the DOS build without launching the playable DOS runtime, equivalent to `make dos-prepare-norun` or `make test-dos-prepare-norun` as appropriate.

## Environment and path model

- `make` runs from Linux shell.
- `dos-prepare.ps1` runs via Windows PowerShell.
- DOS emulator runs on Windows side.

In `dos-prepare.local.ps1`:

- `$source` should be Windows-reachable for Linux-hosted project files.
- `$mountpoint`, `$destination`, `$dospath` should be Windows-visible emulator paths.

The Open Watcom build (`build.bat`) only needs `src/`, `include/`, and `build.bat`. `dos-prepare.ps1` deletes the Windows DOS tree, copies only those paths (separate `robocopy` per directory plus `build.bat`), then strips any stray `.git`, `tests/`, docs, or Linux build junk if an old full mirror left them behind. It launches one waited DOS session for `build.bat`, records the elapsed `build.bat` time in the host console, appends the same line to `build.log` when that file exists, verifies success from the executable and available log output, and then launches the runtime DOS session unless `-NoRun` is set. Add new DOS inputs under `src/` or `include/` (or extend `dos-prepare.ps1` if a new top-level tree is required).

## CI (GitHub Actions)

On `main` and pull requests, CI runs `scripts/ci-stats.sh` (layer check, `make test`, `make test-run`, `make test-unit`, `make test-unit-coverage`, `make test-soak`; see [`.github/workflows/ci.yml`](../.github/workflows/ci.yml)). On pull requests, results are posted or updated in a single PR comment by finding the prior hidden `ci-test-results` marker comment and replacing it, the stats summary is appended to the GitHub Actions job summary, and `ci-stats.json` is uploaded as the sole metrics artifact. CI starts from a clean checkout; for comparable local timings, run `make clean` before the timing-sensitive targets. DOS prep is not run in CI.

Successful `main` CI runs trigger [`.github/workflows/ci-metrics.yml`](../.github/workflows/ci-metrics.yml), which downloads `ci-stats.json`, appends one normalized record into `history.json` on the dedicated `ci-metrics` branch, and pushes that branch back to GitHub. The metrics dashboard at [CI Metrics](ci-metrics.html) reads that branch directly via raw GitHub content, so persistent history is kept out of both PR branches and `main`. The dashboard trends build/test timings, soak benchmark values, per-suite snapshot/unit/soak pass-fail-skip counts, and overall unit branch/line coverage.

When the metrics schema grows, use [`scripts/backfill-ci-history.py`](../scripts/backfill-ci-history.py) to enrich older `ci-metrics/history.json` entries from an archived CI log. It takes the target history file, the target run SHA, and the source log file, then patches only the matching history record.

## Build artifacts

| Target | Output |
|--------|--------|
| `make build` | `./dosmud` (repo root) |
| `make test` | `./dosmud` with `TEST_MODE` (same path; overwrites release binary) |
| `make build-unit` / `make test-unit` / `make test-unit-coverage` / `make test-unit-coverage-verbose` | `tests/unit/build/dosmud_unit`, `*.o`, `*.gcno`, `*.gcda`; `.gcov` under `tests/unit/build/coverage/` (gitignored) |
| `make test-run` | `tests/regression/<name>.output` (gitignored) |
| `make build-soak` / `make test-soak` | `tests/soak/build/dosmud_soak`, `*.o` (gitignored) |
| `make dos-prepare` | `dosmud.exe` and `build.log` in the prepared DOS tree (`$destination` in `dos-prepare.local.ps1`; not mirrored from Linux) |

`make clean` removes `./dosmud`, `tests/unit/build/`, snapshot `*.output`, and legacy root-level unit/coverage junk.

## Manual gameplay verification checklist

The tick HUD line includes `[Atk:n]`; `n` is the flat melee bonus used on combat attacks (level damage bonus plus wielded weapon). Expect the same value when editing snapshot `.expect` files after wield, unwield, or level-up.

1. Start program, confirm initial tick `[T:0]`.
2. Enter `help`; tick remains unchanged. Enter `help craft` (or another topic); tick remains unchanged and a single-topic line prints.
3. Enter invalid command (for example `xyz`); tick remains unchanged.
4. Enter `look`; tick remains unchanged.
5. Enter `wait`; tick increments by exactly 1.
6. Enter `look`, then `move <listed-direction>`; room changes and tick increments by 1.
7. Enter `quit`; process exits cleanly.
8. Bandit hand-over (optional): from a room where a bandit encounter triggers, enter `2`, then `give <item>` for something you carry in the bag **or** your wielded weapon (you can `wield` before replying `2` if the bag is empty); the bandit should leave and that item or wield slot should clear.
