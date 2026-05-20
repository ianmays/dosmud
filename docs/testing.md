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
```

Purpose:

- `make build`: native GCC development build
- `make check-layers`: core/render boundary guard (no `printf` in `src/*.c` except `main.c`, `grendr.c`, and the platform file `platpos.c` or `platdos.c`)
- `make test`: strict deterministic compile (`-Werror`, `-DTEST_MODE`, `-g -O0`); does not run `check-layers`
- `make test-run`: builds the test binary (`make test`), then runs every name in `SNAPSHOT_TESTS` plus `seed_cli` (CLI `--seed` on `smoke.input`; see [Snapshot test files](#snapshot-test-files)). Each step prints `snapshot: <name>`. Finishes with `snapshot tests passed: N/M` (for example `60/60`).
- `make test-unit`: builds and runs the greatest unit suite (`tests/unit/build/dosmud_unit`, `TEST_MODE` only; not linked into release `dosmud`)

## Test layout

```text
tests/
  regression/     # snapshot pairs: <name>.input, <name>.expect (generated <name>.output)
  unit/           # greatest harness: greatest.h, unit_*.c, unit_util.*
    build/        # generated: dosmud_unit, *.o, *.gcno, *.gcda (gitignored)
      coverage/   # *.gcov reports from make test-unit-coverage (gitignored)
```

Snapshot regression lives under [`tests/regression/`](../tests/regression/). Unit sources live under [`tests/unit/`](../tests/unit/). The unit binary and coverage profiles are written to [`tests/unit/build/`](../tests/unit/build/) (ignored by git). Release `dosmud` and snapshot `*.output` are also ignored via [`.gitignore`](../.gitignore).

## Unit tests (greatest)

Phase B coverage ([#95](https://github.com/ianmays/dosmud/issues/95)) uses [greatest](https://github.com/silentbicycle/greatest) **1.5.0** vendored as [`tests/unit/greatest.h`](../tests/unit/greatest.h). Upstream MIT license stays in the header; a small dosmud patch adds quiet-by-default output (`greatest_set_quiet`, `GREATEST_FLAG_QUIET`) in a separate git commit from the unmodified vendor import.

```sh
make test-unit
make test-unit-verbose              # greatest suite/test progress only
make test-unit-verbose-gameplay     # greatest progress + gameplay render text
make test-unit-coverage             # runs test-unit, then compact branch/line % table
make test-unit-coverage-verbose     # same tests, full gcov block per module
```

- Binary: `tests/unit/build/dosmud_unit` (all gameplay modules except `main.c`, plus `tests/unit/unit_*.c`)
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

**In-scope modules (branch coverage target ~90%+):** `command`, `invent`, `combat`, `game`, `genc`, `wanderer`, `dialogue`, `gatmos`, `world`, `gprog`, `items`, `testharn`

**Out of scope for the unit coverage bar:** `grendr`, `txtres`, `main`, `platpos` / `platdos` (presentation and platform glue; snapshots cover render text)

**Harness-only fixture:** `bag_full_gate` - applies `game_inv_bag_add` without resetting baseline; returns fixture failure (`-2`) when the bag is already full (used by unit tests for `testharn_apply` error paths)

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
| `bandit_combat_salve_ready` | Combat turn 1, salve in bag + inject for enemy turn after salve |
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

5. **World graph** (`TEST_MODE` only) - `@fixture world_boot` passes `th_world_snapshot_*` tables from [`src/testharn.c`](../src/testharn.c) into `world_apply_graph` (same pass-in style as roll data into `game_roll_inject_begin`). Unlike rolls, the graph is **not** cleared by `game_reset_fixture_baseline`; it persists until the next `world_boot` or a new process. When `world_init` changes and you want `world_boot` to match the default test seed layout, refresh those tables in testharn (they do not track generator changes automatically). Unit tests use a **duplicate** copy of the same seed-1234 graph in [`tests/unit/unit_util.c`](../tests/unit/unit_util.c) (`unit_world_*` via `unit_world_boot_graph`); update **both** when the default layout changes.

Bandit intimidate in gameplay uses `game_roll_percent` (not raw `rand()`), so intimidate snapshots stay on the inject path.

Add new fixtures in [`src/testharn.c`](../src/testharn.c) and document them here. `testharn` is linked only for `make test` / `dos-prepare MODE=TEST_MODE` and `make test-unit`, not for `make build`.

| Fixture | Notes |
|---------|--------|
| `bag_full_gate` | No baseline reset; fails setup when `game_inv_bag_add` cannot store `ITEM_STICK` (bag already full) |

### Adding a snapshot test

1. Add `tests/regression/<name>.input` (and prefer one scenario per file).
2. Use `@fixture` for setup; add inject in the fixture or via a `*_ready` fixture when outcomes must be fixed.
3. Use `quiet_explore` when the test calls `wait` or `move`.
4. Run `make test && make test-run` (or `./dosmud < tests/regression/<name>.input > tests/regression/<name>.output`) and copy or diff against `tests/regression/<name>.expect`.
5. Add `<name>` to `SNAPSHOT_TESTS` in the [Makefile](../Makefile) (`seed_cli` stays separate: it uses `--seed` with `smoke.input`).
6. Document new fixtures in this file.

### Snapshot test files

Each process run uses one `.input` file until `quit`. `make test-run` runs `SNAPSHOT_TESTS` (includes `smoke`), then `seed_cli`.

**Core / inventory (also in `SNAPSHOT_TESTS`):** `smoke`, `bandit_handover`, `bandit_wielded_give`, `area_items`, `map`, `equipment`, `craft_wielded`.

**Movement / time:** `walk_north`, `walk_map`, `wait_tick`.

**NPC talk:** `frog_replies`, `watchman_talk`, `wanderer_replies`, `wanderer_talk_blocked`, `herbalist_talk`, `archivist_talk`, `talk_nobody`.

**Eat / use:** `use_salve`, `use_torch`, `use_spear`, `use_stone`, `eat_berry`, `eat_fish`, `eat_not_edible`, `eat_missing`.

**Inspect:** `inspect_rustle`, `inspect_creak`, `inspect_water`, `inspect_grit`, `inspect_none`, `inspect_wrong`.

**Combat:** `combat_defend`, `combat_salve`, `combat_no_salve`, `combat_invalid`, `combat_take_blocked`, `combat_victory_xp`, `level_up`.

**Loot:** `loot_spear`, `loot_stick`, `loot_berry`, `loot_herb`, `loot_fish`, `loot_empty`, `loot_stripped`, `loot_bag_full`.

**Bandit dialogue:** `bandit_fight`, `bandit_intimidate_ok`, `bandit_intimidate_fail`, `bandit_bag_empty`.

**Meta / inventory:** `unknown_cmd`, `cannot_move`, `give_wrong_context`, `reply_nobody`, `reply_invalid`, `craft_salve`, `craft_unknown`, `take_nothing`, `take_wrong_item`.

## DOS/Open Watcom validation path

Use PowerShell-driven DOS prep from Linux host shell to build and sync the DOS tree:

```sh
make dos-prepare
```

Start DOS and launch the existing DOS executable without rebuilding or refreshing the tree:

```sh
make dos-run
```

`make dos-run` expects a previously prepared DOS tree. Run `make dos-prepare` first if the mirrored DOS files or executable are missing.

When you add or remove `src\*.c` files, update `Makefile` (`SRC` or `TEST_SRC`) and `build.bat`. For the Open Watcom path, keep every `wcl` and `wlib` line under the COMMAND.COM length limit (about 127 characters): gameplay sources are packed into `gameplay.lib` via several short `wlib` calls; the final `wcl` link lists `main.obj`, `platdos.obj`, `gameplay.lib`, plus the other `.obj` files. `TEST_MODE` compiles `testharn.c` to `tharn.obj` and appends it with a separate `wlib gameplay.lib +tharn.obj` line. Use `goto` labels in `build.bat` for conditionals; parenthesized `if (...)` blocks break under COMMAND.COM.

Deterministic DOS validation:

```sh
make dos-prepare MODE=TEST_MODE
```

Runtime seed (native or DOS build): the startup banner always prints the active seed, for example `dosmud (seed 1234)`. In `TEST_MODE` the default is `CFG_TEST_RAND_SEED` unless overridden on the command line:

```sh
./dosmud --seed 1234
```

Invalid flags print `usage: dosmud [--seed <unsigned>]` to stderr and exit with status 1. Seed values must be decimal, non-negative, and at most `CFG_SEED_CLI_MAX` (4294967295); leading `+`/`-` and out-of-range values are rejected.

## Combined cross-path checks

When changing build flow/tooling or other high-risk runtime behavior:

```sh
make build-all
make test-all
```

These targets intentionally exercise DOS prep/invocation and native GCC flow together.

## Environment and path model

- `make` runs from Linux shell.
- `dos-prepare.ps1` runs via Windows PowerShell.
- DOS emulator runs on Windows side.

In `dos-prepare.local.ps1`:

- `$source` should be Windows-reachable for Linux-hosted project files.
- `$mountpoint`, `$destination`, `$dospath` should be Windows-visible emulator paths.

The Open Watcom build (`build.bat`) only needs `src/`, `include/`, and `build.bat`. `dos-prepare.ps1` deletes the Windows DOS tree, copies only those paths (separate `robocopy` per directory plus `build.bat`), then strips any stray `.git`, `tests/`, docs, or Linux build junk if an old full mirror left them behind. Add new DOS inputs under `src/` or `include/` (or extend `dos-prepare.ps1` if a new top-level tree is required).

## CI (GitHub Actions)

On `main` and pull requests, CI runs `scripts/ci-test-report.sh` (layer check, `make test`, `make test-run`, `make test-unit`, `make test-unit-coverage`; see [`.github/workflows/ci.yml`](../.github/workflows/ci.yml)). On pull requests, results are posted or updated in a single PR comment (`comment-tag: ci-test-results`). DOS prep is not run in CI.

## Build artifacts

| Target | Output |
|--------|--------|
| `make build` | `./dosmud` (repo root) |
| `make test` | `./dosmud` with `TEST_MODE` (same path; overwrites release binary) |
| `make test-unit` / `make test-unit-coverage` / `make test-unit-coverage-verbose` | `tests/unit/build/dosmud_unit`, `*.o`, `*.gcno`, `*.gcda`; `.gcov` under `tests/unit/build/coverage/` (gitignored) |
| `make test-run` | `tests/regression/<name>.output` (gitignored) |
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
