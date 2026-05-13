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

## Platform boundary rule

Core gameplay must not depend on platform APIs or rendering details.

Good:

```c
game_process_input(game, line);
```

Bad in gameplay systems:

```c
printf("Player moved.\n");
```

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

## Subsystem responsibilities

## `main`

- startup
- main loop orchestration
- input/timing integration

## `game`

- top-level gameplay orchestration
- command routing
- world update sequencing
- `game_is_busy_dialogue` gating for ambient encounters (shared with encounter and wanderer entry)

Gameplay slices live beside `game.c` as plain C translation units (no extra framework):

- [`progression.c`](../src/progression.c) - XP and level-up rewards (`game_xp_to_next_level`, `progression_gain_xp`)
- [`combat.c`](../src/combat.c) - combat start, player reply resolution, enemy turn
- [`encounter.c`](../src/encounter.c) - ambient bandit encounter open state
- [`wanderer.c`](../src/wanderer.c) - traveler movement and encounter flow
- [`dialogue.c`](../src/dialogue.c) - pond frog lines and NPC id hint for room look
- [`atmosphere.c`](../src/atmosphere.c) - initial room items, ambient rolls, animal noise, inspect focus hooks

`game` stays orchestration; new behaviour should land in the owning slice above rather than re-centralising into `game.c`.

## `command`

- parse raw text into structured commands
- keep parsing separate from execution/mutation

## `world`

- room graph data and connectivity
- procedural world generation
- movement validation
- logical map coordinates assigned when rooms are linked (used only for the exploration map display)

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
- `TEST_MODE` seeds libc RNG with `CFG_TEST_RAND_SEED` for deterministic snapshot output

## `grendr`

- text rendering only
- no gameplay mutation
- player-facing lines and format strings come from [`txtres.c`](../src/txtres.c) (`TXT_*` constants and `g_room_*` arrays), not scattered literals

## `txtres`

- single home for static player-facing copy
- exported globals, not thin getters
- functions only where selection matters

## `invent`

- bag/inventory state mutation
- item use and crafting behavior
- wield/unwield commands track `weapon_equipped` on `GameState`; a wielded weapon is not stored in `bag[]` (it occupies the hand slot only until unwield, drop, or bandit handover moves it)
- combat adds `item_weapon_damage_bonus` from `weapon_equipped` when the player attacks; it does not require the weapon id to appear in the bag

## `items`

- item metadata and lookup

## Core data ownership

## `GameState`

`GameState` is the primary simulation container. Gameplay systems should mutate it explicitly and avoid shadow copies.

## `World` and `Room`

`World` stores room graph data. `Room` stores room metadata, exits, and ambient state.

## Determinism rules

- seed randomness once at startup
- evolve simulation via commands and background ticks
- never tie simulation correctness to render cadence

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
