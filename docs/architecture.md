# Architecture

DOSMUD is a DOS-first adventure/RPG written in ANSI C89.

## Core philosophy

- deterministic simulation for identical seed + inputs
- clear subsystem boundaries
- explicit procedural code over heavy abstraction
- low memory overhead and DOS-friendly constraints

The codebase intentionally avoids:

- dynamic allocation by default
- hidden globals
- object-emulation frameworks and ECS-style designs

## Engine layers (core / render / platform)

The codebase splits into three layers. Gameplay and simulation modules must stay in **core**; only **render** and **platform** may touch terminal I/O or OS APIs.

### Core (gameplay simulation)

Owns state mutation, command handling, world ticks, combat, inventory logic, and procedural systems.

Modules include `game`, `command`, `world`, `invent`, `combat`, `gatmos`, `gprog`, `genc`, `wanderer`, `dialogue`, and `items`.

Core must **not** use:

- `printf` or other terminal output (call `render_*` in `grendr` instead)
- DOS, SDL, or platform timing/input APIs
- `#ifdef __WATCOMC__` or similar platform switches

Good:

```c
game_process_input(game, line);
render_msg_moved(world_dir_name(cmd->dir));
```

Bad in core:

```c
printf("Player moved.\n");
```

### Render (`grendr`, `txtres`)

Presentation only: room art, HUD, combat text, inventory messages, and exploration map output.

- `grendr` is the only gameplay-adjacent module that may call `printf`
- `txtres` holds static copy; it does not print

Core calls `render_*` after mutating `GameState`; render never changes simulation state.

### Newline and spacing

Player-facing copy lives in `txtres`; `grendr` owns when a blank line appears before output.

- **`txtres`:** each string ends with exactly one `\n`. Do not embed a leading `\n` in copy (the main prompt in `main` is the exception).
- **`grendr`:** use the tier helpers in [`grendr.c`](../src/grendr.c):
  - **Line** - inline messages (combat, errors): print copy only.
  - **Paragraph** - atmosphere primaries, animal noise: `render_paragraph()` (one gap, then copy).
  - **Scene** - room look art, encounters, NPC portraits: `render_gap()` once before the art block.
  - **Continuation** - atmosphere follow-ups (berry/reed), dialogue after art: no extra gap.
- **ASCII art:** the first row must be drawing, not a blank spacer row. Section breaks come from `render_gap()`, not padding lines at the top of art.

### Platform (`main`, `platform.h`, `platdos.c` / `platpos.c`)

[`include/platform.h`](../include/platform.h) defines the portable boundary:

- `plat_poll_line` - non-blocking stdin poll (DOS `kbhit`/`getch` or POSIX `select`)
- `plat_time_now` - wall-clock seconds for idle ticks
- `plat_seed_rng` - applies `srand((unsigned int)seed)`; `main.c` chooses a `u32` seed (`CFG_TEST_RAND_SEED`, wall clock, or `--seed`). `GameState.seed` stores the full `u32`; libc may use fewer bits (for example 16-bit `unsigned int` on DOS)

Implementations are split by toolchain (FAT 8.3 basenames):

- [`src/platdos.c`](../src/platdos.c) - Open Watcom / DOS (`build.bat` links `platdos.obj`)
- [`src/platpos.c`](../src/platpos.c) - GCC / POSIX (`Makefile` links `platpos.c`)

[`src/main.c`](../src/main.c) orchestrates the main loop and may use `printf` for shell-level prompts and banners. It must not include `conio.h`, `dos.h`, or other platform headers directly.

Run `make check-layers` before opening a PR (or use `make test-all`, which runs it first). That target fails if `printf` appears in any `src/*.c` other than `main.c`, `grendr.c`, `platdos.c`, or `platpos.c`. `make test` compiles only and does not run the guard.

## High-level flow

```text
Input
  ->
Command Parsing
  ->
Gameplay Logic
  ->
World Tick Advance
  ->
Rendering
```

Commands mutate game state, world ticks mutate simulation state, and rendering only displays state.

## Configuration (`config.h`)

[`include/config.h`](../include/config.h) is the compile-time home for:

- structural limits (`CFG_ROOM_MAX`, `CFG_BAG_MAX`, `CFG_AREA_ITEM_SLOTS` ground slots per room, buffers, etc.)
- gameplay tuning (combat and bandit corpse loot thresholds, progression, ambient systems, wanderer timing)
- world-generation numeric policy (`world_init` counts and loop bounds)
- `WORLD_ROOM_*` room IDs

Conventions:

- add new gameplay/procedural tuning knobs here as `CFG_*` macros
- keep related values grouped and commented
- separate gameplay tuning from main-loop/test-harness settings
- distinguish tuning for NPC corpse loot (`CFG_COMBAT_CORPSE_LOOT_*`, portable items) from ambient room finds (`CFG_ROOM_SPAWN_*`, terrain-driven junk like stone)
- `TEST_MODE` defaults libc RNG to `CFG_TEST_RAND_SEED` for deterministic snapshot output; override with `dosmud --seed <unsigned>`
- roll-inject limits and snapshot roll constants (`CFG_ROLL_INJECT_*`, `CFG_TEST_*`) are defined only under `#ifdef TEST_MODE` in `config.h`

### Test harness (`testharn`, `TEST_MODE` only)

[`tests/harness/testharn.c`](../tests/harness/testharn.c) lives at the `main` edge (not core simulation). It applies `@fixture` and `@seed` lines from snapshot `.input` files by calling `game_reset_fixture_baseline` plus existing inventory, encounter, and `render_*` APIs (same paths as normal play). Shared seed-1234 world layout tables live in [`tests/harness/th_world.c`](../tests/harness/th_world.c). After a successful harness directive, `main.c` calls `plat_seed_rng(game.seed)` so libc RNG matches the stored seed. Fixtures cover bandit dialogue/combat, room placement, bag contents, inspect focus, corpse loot, combat-ready inject queues, and `quiet_explore` (`test_quiet_ticks` + wanderer off). The `bandit_combat_turn1_resolve` fixture also calls `game_roll_inject_begin` and `combat_resolve_reply` so the `equipment` snapshot exercises real combat without a scripted `1`. `@seed` sets `GameState.seed` mid-file for libc RNG stream isolation. Release builds (`make build`) do not link the harness. See [testing](testing.md#test-fixtures-test_mode-only).

## Base types (`base.h`)

[`include/base.h`](../include/base.h) defines portable unsigned aliases used in simulation state:

- `u8` - byte storage (explored-map flags, `map_ready`, safe `char` casts for `ctype` calls)
- `u16` - 16-bit values when needed (defined for future use; compile-time `sizeof` guard)
- `u32` - tick counters, seeds, and other monotonic world-time fields (`GameState.tick`, `world_step` argument); maps to `unsigned long` (32-bit on DOS/Open Watcom, may be wider on LP64 Linux hosts)

Conventions:

- use `int` for gameplay quantities, room ids, and boolean-style flags unless a fixed width is required
- keep `config.h` for limits and tuning; do not fold typedefs into `config.h`
- see [contributor guide](contributor-guide.md) for ANSI C89 / Open Watcom portability rules

## Subsystem responsibilities

### `main`

- startup
- main loop orchestration
- input/timing integration
- `TEST_MODE`: delegates `@fixture` and `@seed` lines to `testharn`

### `game`

- top-level gameplay orchestration
- command routing
- world update sequencing
- explicit game modes in [`game.h`](../src/game.h): `GameMode` (explore, dialogue, combat), `DialogueKind` for the active dialogue when in dialogue mode (room NPCs including the pond frog, wanderer, enemy), and `CombatState` for combat-only fields
- mode transitions via `game_set_mode_explore`, `game_set_mode_dialogue`, and `game_set_mode_combat` (only one major mode at a time)
- `game_is_busy_dialogue` returns true whenever `mode != GAME_MODE_EXPLORE` (ambient encounters, idle background ticks)
- `game_roll_spread` and `game_roll_percent` centralize gameplay draws used for combat, corpse loot, kill XP, and bandit intimidate (`rand()` when inject is inactive)
- `TEST_MODE` only: `game_roll_inject_*` and `test_quiet_ticks` on `GameState`; when `test_quiet_ticks` is set, `advance_world_tick` skips ambient atmosphere, animal noise, bandit ambush, and wanderer movement (see [quiet ticks](testing.md#quiet-ticks-test_quiet_ticks-test_mode-only))
- [`gatmos.c`](../src/gatmos.c) and wanderer return timing still use raw `rand()` in normal play; world generation uses `rand()` at init

Gameplay slices live beside `game.c` as plain C translation units (no extra framework):

- [`gprog.c`](../src/gprog.c) - XP and level-up rewards (`game_xp_to_next_level`, `progression_gain_xp`; FAT 8.3-safe basename)
- [`combat.c`](../src/combat.c) - combat start, player reply resolution, enemy turn (randomness via `game_roll_spread` / `game_roll_percent`, not `rand()`)
- [`genc.c`](../src/genc.c) - ambient bandit encounter entry, bandit reply and handover commands (`genc_cmd_reply`, `genc_cmd_give`; FAT 8.3-safe basename)
- [`wanderer.c`](../src/wanderer.c) - traveler movement, encounter flow, wanderer reply command (`wanderer_cmd_reply`)
- [`dialogue.c`](../src/dialogue.c) - pond frog lines, NPC id hint for room look, talk and room-NPC reply commands (`dialogue_cmd_talk`, `dialogue_cmd_reply`)
- [`gatmos.c`](../src/gatmos.c) - initial room items, ambient rolls, animal noise, inspect command (`gatmos_cmd_inspect`; FAT 8.3-safe basename)

When a slice exposes new command or state entry points, add tests in the matching `tests/unit/unit_*.c` file (see [When to add or update tests](testing.md#when-to-add-or-update-tests)).

New `src/*.c` and `src/*.h` basenames must stay within **classic FAT 8+3** (at most eight characters before `.c` or `.h`) so MS-DOS 5.x-6.x style volumes and the Open Watcom DOS build can open them reliably. Existing examples: `grendr.*`, `invent.*`, `gprog.*`, `gatmos.*`, `genc.*`.

`game` stays orchestration: `apply_command` in [`game.c`](../src/game.c) routes by mode and command type to the slices above via static helpers (`game_cmd_meta`, `game_cmd_move`, `game_cmd_inventory`, `game_cmd_reply`). New behaviour should land in the owning slice rather than re-centralising into `game.c`.

### `command`

- parse raw text into structured commands
- keep parsing separate from execution/mutation

### `world`

- room graph data and connectivity
- procedural world generation
- movement validation
- logical map coordinates assigned when rooms are linked (used only for the exploration map display)

### `grendr`

- text rendering only
- art is intentionally compact to work well with 25 line displays (DOS standard)
- no gameplay mutation
- player-facing lines and format strings come from [`txtres.c`](../src/txtres.c) (`TXT_*` constants and `g_room_*` arrays), not scattered literals
- newline tiers (`render_gap`, `render_paragraph`, and related rules): [Newline and spacing](#newline-and-spacing)

### `txtres`

- single home for static player-facing copy
- trailing `\n` only on copy strings; see [Newline and spacing](#newline-and-spacing)
- exported globals, not thin getters
- functions only where selection matters

### `invent`

- bag/inventory state mutation
- item use and crafting behavior
- wield/unwield commands track `weapon_equipped` on `GameState`; a wielded weapon is not stored in `bag[]` (it occupies the hand slot only until unwield, drop, or bandit handover moves it)
- combat adds `item_weapon_damage_bonus` from `weapon_equipped` when the player attacks; it does not require the weapon id to appear in the bag

### `items`

- item metadata and lookup

## Core data ownership

### `GameState`

`GameState` is the primary simulation container. Gameplay systems should mutate it explicitly and avoid shadow copies. In `TEST_MODE` builds only, it may include the roll-inject queue and `test_quiet_ticks`; release `GameState` has neither.

### `World` and `Room`

`World` stores room graph data. `Room` stores room metadata, exits, and ambient state.

## Determinism rules

- seed randomness once at startup (`plat_seed_rng` from `main`; combat uses `game_roll_*`, which calls `rand()` when inject is inactive)
- evolve simulation via commands and background ticks
- never tie simulation correctness to render cadence
- snapshot tests under `tests/regression/` combine fixtures (direct state), optional roll inject (`TEST_MODE` only), and a fixed default seed for remaining libc `rand()` paths; see [fixture design trade-offs](testing.md#fixture-design-trade-offs)
- greatest unit tests under `tests/unit/` assert `GameState` and parse results (`make test-unit`); see [unit tests](testing.md#unit-tests-greatest)

## ANSI C89/C90 compatibility rules

Target constraints:

- ANSI C89 / ISO C90
- GCC compatible
- Open Watcom compatible

Allowed/preferred:

- fixed arrays
- explicit structs and procedural flow
- compile-time limits

Avoid:

- C99/C11 features (`//` comments, mixed declarations/statements, declarations inside `for`)
- recursion
- heavy heap allocation unless justified
- compiler-specific extensions

## Feature development guidance

When adding features:

- keep subsystem ownership boundaries clean
- keep deterministic behavior intact
- prefer simple, explicit code over clever abstractions
- preserve ANSI C portability
