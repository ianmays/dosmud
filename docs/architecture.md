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

Simulation steps append fixed-size `GameEvent` records through the `gout` queue while mutating `GameState`. Core does not print directly; callers may inspect those records headlessly or hand them to the DOSMUD render adapter.

Within core, keep the ownership split explicit:

- **Engine** - deterministic `GameState` stepping (`game_describe_current_room`, `game_process_input`, `game_background_step`) plus `GameEvent` / `gout` records. The engine mutates state and emits semantic output requests, but never performs terminal I/O.
- **Game logic** - dosmud-specific rules and content that plug into that stepping surface: command routing in `game`, room/world rules, and the gameplay slices (`combat`, `invent`, `dialogue`, `genc`, `wanderer`, `gatmos`, `gprog`, `items`).

[`src/game.h`](../src/game.h) defines the engine-facing stepping surface and persistent simulation state; [`src/gout.h`](../src/gout.h) defines the fixed-size event queue that carries engine results to the render edge.

Command and navigation stepping in `game.c` emit generic `GameEventKind` values (handled in `grendr`): `GAME_EVENT_MOVE` and `GAME_EVENT_ROOM_LOOK` (successful move plus room look), `GAME_EVENT_MAP`, `GAME_EVENT_HELP` (`arg0` = `CMD_HELP_*` topic), `GAME_EVENT_WAIT`, `GAME_EVENT_CANNOT_MOVE` (`text` = direction name), and `GAME_EVENT_UNKNOWN_COMMAND`. Inventory and item handlers in `invent.c` emit `GAME_EVENT_ITEM_RESULT`, `GAME_EVENT_BAG_VIEW`, `GAME_EVENT_CRAFT_RESULT`, and `GAME_EVENT_EQUIP_RESULT` (payload contract in [`gout.h`](../src/gout.h)). Combat and progression in [`combat.c`](../src/combat.c) and [`gprog.c`](../src/gprog.c) emit `GAME_EVENT_COMBAT` (`GameEventCombatPhase` in `arg0`), `GAME_EVENT_XP_GAIN`, and `GAME_EVENT_STAT_CHANGE` (payload contract in [`gout.h`](../src/gout.h)). Dialogue and encounter slices in [`dialogue.c`](../src/dialogue.c), [`wanderer.c`](../src/wanderer.c), and [`genc.c`](../src/genc.c) emit `GAME_EVENT_DIALOGUE`, `GAME_EVENT_ENCOUNTER`, and `GAME_EVENT_DIALOGUE_GUARD`; modal guards in `game.c` emit `GAME_EVENT_DIALOGUE_GUARD` when reply or handover context blocks a command (payload contract in [`gout.h`](../src/gout.h)). Ambient and inspect output in [`gatmos.c`](../src/gatmos.c) emit `GAME_EVENT_ENVIRONMENT`, `GAME_EVENT_AMBIENT_NOISE`, `GAME_EVENT_ITEM_PRESENCE`, and `GAME_EVENT_OBSERVATION` on world ticks and the inspect command (payload contract in [`gout.h`](../src/gout.h)). All production slices append generic `GameEventKind` values only; `grendr` dispatches them to player-visible text.

Core must **not** use:

- `printf`, `render_*`, or other terminal output APIs
- DOS, SDL, or platform timing/input APIs
- `#ifdef __WATCOMC__` or similar platform switches

Good:

```c
GameEventQueue out;

game_event_queue_reset(&out);
game_process_input(game, line, &out);
game_render_output(game, &out);
```

Bad in core:

```c
printf("Player moved.\n");
```

### Render (`txtres`, `fmt`, `grendr`)

Presentation only: room art, HUD, combat text, inventory messages, and exploration map output.

- **`txtres`** holds static copy; it does not print
- **`fmt`** builds player-visible strings from `GameState` into caller buffers (no terminal I/O); logic-heavy formatting (for example aggregated bag lists) lives here
- **`grendr`** is the only gameplay-adjacent module that may call `printf`; it prints `fmt` output, applies newline/spacing tiers, draws ASCII art, and acts as the DOSMUD text-render adapter over the generic `GameEvent` queue (room/move, command/navigation, inventory/item, combat/progression, dialogue/encounter, ambient/inspect).

Platform or frontend code runs simulation first, then hands the resulting `GameEvent` records to render; render never changes simulation state.

### Newline and spacing

Player-facing copy lives in `txtres`; `grendr` owns when a blank line appears before output.

- **`txtres`:** each string ends with exactly one `\n`. Do not embed a leading `\n` in copy (the main prompt in `main` is the exception).
- **`grendr`:** use the tier helpers in [`grendr.c`](../src/grendr.c):
  - **Line** - inline messages (combat, errors): print copy only.
  - **Paragraph** - atmosphere primaries, animal noise: `render_paragraph()` (one gap, then copy).
  - **Scene** - room look art, encounters, NPC portraits: `render_gap()` once before the art block.
  - **Continuation** - atmosphere follow-ups (berry/reed), dialogue after art: no extra gap.
- **ASCII art:** the first row must be drawing, not a blank spacer row. Section breaks come from `render_gap()`, not padding lines at the top of art.

### Platform (`main`, `platform.h`, `platdos.c` / `platpos.c` / `platwin.c`)

[`include/platform.h`](../include/platform.h) defines the portable boundary:

- `plat_poll_line` - non-blocking stdin poll (DOS `kbhit`/`getch`, Windows console `_kbhit`/`_getch`, or POSIX `select`)
- `plat_time_now` - wall-clock seconds for idle ticks
- `plat_seed_rng` - applies `srand((unsigned int)seed)`; `main.c` chooses a `u32` seed (`CFG_TEST_RAND_SEED`, wall clock, or `--seed`). `GameState.seed` stores the full `u32`; libc may use fewer bits (for example 16-bit `unsigned int` on DOS)

Implementations are split by toolchain (FAT 8.3 basenames):

- [`src/platdos.c`](../src/platdos.c) - Open Watcom / DOS (`build.bat` links `platdos.obj`)
- [`src/platpos.c`](../src/platpos.c) - GCC / POSIX (`Makefile` links `platpos.c`)
- [`src/platwin.c`](../src/platwin.c) - Windows console path for WSL cross-builds (`make build-win` / `make test-win`)

[`src/main.c`](../src/main.c) orchestrates the main loop and may use `printf` for shell-level prompts and banners. It must not include `conio.h`, `dos.h`, or other platform headers directly.

Run `make check-layers` before opening a PR (or use `make test-all`, which runs it first). That target fails if `printf` appears in any `src/*.c` other than `main.c`, `grendr.c`, `platdos.c`, `platpos.c`, or `platwin.c`. `make test` compiles only and does not run the guard.

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
- gameplay tuning (combat and bandit corpse loot thresholds, food/salve heal amounts, progression, ambient systems, wanderer timing)
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

[`tests/harness/testharn.c`](../tests/harness/testharn.c) lives at the `main` edge (not core simulation). It applies `@fixture` and `@seed` lines from snapshot `.input` files by calling `game_reset_fixture_baseline` plus real gameplay APIs, usually capturing the event queue into a local buffer and either dropping it with `harness_drop_output` or rendering it through `game_render_output` when the snapshot needs the visible prompt or encounter text. A few edge prompts still use direct `render_*` calls where the harness is intentionally reproducing shell-facing output that is not part of a normal command or tick step. Shared seed-1234 world layout tables live in [`tests/harness/th_world.c`](../tests/harness/th_world.c). After a successful harness directive, `main.c` calls `plat_seed_rng(game.seed)` so libc RNG matches the stored seed. Fixtures cover bandit dialogue/combat, room placement, bag contents, inspect focus, corpse loot, combat-ready inject queues, and `quiet_explore` (`test_quiet_ticks` + wanderer off). The `bandit_combat_turn1_resolve` fixture also calls `game_roll_inject_begin` and `combat_resolve_reply` so the `equipment` snapshot exercises real combat without a scripted `1`. `@seed` sets `GameState.seed` mid-file for libc RNG stream isolation. Release builds (`make build`) do not link the harness. See [testing](testing.md#test-fixtures-test_mode-only).

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
- headless step surface: `game_describe_current_room`, `game_process_input`, and `game_background_step` mutate `GameState` and append `GameEvent` records supplied by the caller
- explicit game modes in [`game.h`](../src/game.h): `GameMode` (explore, dialogue, combat), `DialogueKind` for the active dialogue when in dialogue mode (room NPCs including the pond frog, wanderer, enemy), and `CombatState` for combat-only fields
- mode transitions via `game_set_mode_explore`, `game_set_mode_dialogue`, and `game_set_mode_combat` (only one major mode at a time)
- `game_is_busy_dialogue` returns true whenever `mode != GAME_MODE_EXPLORE` (ambient encounters, idle background ticks)
- `game_roll_spread` and `game_roll_percent` centralize gameplay draws used for combat, corpse loot, kill XP, and bandit intimidate (`rand()` when inject is inactive)
- `TEST_MODE` only: `game_roll_inject_*` and `test_quiet_ticks` on `GameState`; when `test_quiet_ticks` is set, `advance_world_tick` skips ambient atmosphere, animal noise, bandit ambush, and wanderer movement (see [quiet ticks](testing.md#quiet-ticks-test_quiet_ticks-test_mode-only))
- [`gatmos.c`](../src/gatmos.c) and wanderer return timing still use raw `rand()` in normal play; world generation uses `rand()` at init

Gameplay slices live beside `game.c` as plain C translation units (no extra framework):

- [`gprog.c`](../src/gprog.c) - XP and level-up rewards (`game_xp_to_next_level`, `progression_gain_xp`); queues `GAME_EVENT_XP_GAIN` and `GAME_EVENT_STAT_CHANGE` via `gout` (FAT 8.3-safe basename)
- [`combat.c`](../src/combat.c) - combat start, player reply resolution, enemy turn; queues `GAME_EVENT_COMBAT` phases via `gout` (randomness via `game_roll_spread` / `game_roll_percent`, not `rand()`)
- [`genc.c`](../src/genc.c) - ambient bandit encounter open state (FAT 8.3-safe basename)
- [`wanderer.c`](../src/wanderer.c) - traveler movement and encounter flow
- [`dialogue.c`](../src/dialogue.c) - pond frog lines and NPC id hint for room look
- [`gatmos.c`](../src/gatmos.c) - initial room items, ambient rolls, animal noise, inspect focus hooks (FAT 8.3-safe basename)

When a slice exposes new command or state entry points, add tests in the matching `tests/unit/unit_*.c` file (see [When to add or update tests](testing.md#when-to-add-or-update-tests)).

New `src/*.c` and `src/*.h` basenames must stay within **classic FAT 8+3** (at most eight characters before `.c` or `.h`) so MS-DOS 5.x-6.x style volumes and the Open Watcom DOS build can open them reliably. Existing examples: `grendr.*`, `invent.*`, `gprog.*`, `gatmos.*`, `genc.*`.

`game` stays orchestration; new behaviour should land in the owning slice above rather than re-centralising into `game.c`. [`game_heal_player`](../src/game.c) applies capped HP heals and is used by inventory eat/salve and combat salve reply 3.

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
- calls [`fmt.c`](../src/fmt.c) for logic-heavy strings, then prints; static copy from [`txtres.c`](../src/txtres.c) (`TXT_*` constants and `g_room_*` arrays), not scattered literals
- newline tiers (`render_gap`, `render_paragraph`, and related rules): [Newline and spacing](#newline-and-spacing)

### `fmt`

- pure string formatting from `GameState`; writes into caller-provided buffers
- no `printf` or gameplay mutation
- unit-tested directly in [`tests/unit/unit_fmt.c`](../tests/unit/unit_fmt.c) (see [`docs/testing.md`](testing.md))

### `txtres`

- single home for static player-facing copy
- trailing `\n` only on copy strings; see [Newline and spacing](#newline-and-spacing)
- exported globals, not thin getters
- functions only where selection matters

### `invent`

- bag/inventory state mutation; command outcomes queue generic `GameEvent` records (`gout`) for `grendr` to render
- item use and crafting behavior
- `eat` and inventory `use salve` restore HP via `game_heal_player`; at max HP the item is still consumed but no heal is applied (player sees an already-full message)
- wield/unwield commands track `weapon_equipped` on `GameState`; a wielded weapon is not stored in `bag[]` (it occupies the hand slot only until unwield, drop, or bandit handover moves it)
- combat adds `item_weapon_damage_bonus` from `weapon_equipped` when the player attacks; it does not require the weapon id to appear in the bag

### `items`

- item metadata and lookup
- `item_food_heal_amount` for edible heal values (`CFG_BERRY_HEAL_AMOUNT`, `CFG_FISH_HEAL_AMOUNT`)

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
