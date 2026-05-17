# DOSMUD ANSI C89 Development Plan

## Core Project Rules

The project should treat these as non-negotiable:

```text
Language target:
- ANSI C89 / ISO C90
- OpenWatcom compatible
- GCC compatible
- No compiler extensions required
```

The project should prioritize:

- deterministic gameplay
- procedural ANSI C architecture
- explicit ownership and state flow
- DOS/OpenWatcom portability
- maintainability over rapid expansion
- subsystem isolation
- deterministic testing

The project should avoid:

- framework-style architecture
- object-emulation patterns
- ECS-style abstractions
- hidden ownership
- platform/render leakage into gameplay
- premature complexity

---

# Current Project Priority

The project is currently in an architectural consolidation phase.

Highest-value work:

1. clarify subsystem ownership
2. reduce architecture drift
3. improve deterministic testing
4. preserve ANSI C89 portability
5. improve workflow/tooling discipline

Large-scale gameplay/content expansion should remain secondary until the core architecture stabilizes.

---

## Hard Constraints

### Allowed
- fixed arrays
- static/global storage
- procedural architecture
- explicit state passing
- simple structs
- deterministic systems

### Avoid
- dynamic allocation
- recursion
- compiler-specific features
- C99/C11 features
- hidden globals
- giant monolithic modules

---

# Phase 1 - Structural Cleanup + ANSI C89 Enforcement

## Primary Goals

- reduce complexity concentration
- preserve portability
- clarify subsystem ownership

---

## #42 - Split `game.c`

Done ✅.

Highest-priority architecture task.

Orchestration stays in `game.c`; combat, dialogue (pond + NPC hint), atmosphere (`gatmos`), wanderer, progression (`gprog`), and ambient encounter entry (`genc`) live in dedicated translation units (see `docs/architecture.md`).

Target structure:

```text
src/
    game.c
    combat.c
    dialogue.c
    gatmos.c
    wanderer.c
    gprog.c
    genc.c
```

### `game.c`
Only:
- high-level orchestration
- tick sequencing
- command routing
- mode transitions

### `combat.c`
Own:
- combat state
- combat flow
- enemy turns
- combat rewards

### `dialogue.c`
Own:
- NPC dialogue
- reply handling
- dialogue state

### `wanderer.c`
Own:
- wanderer movement
- encounter triggering
- separation logic

### `gatmos.c` (atmosphere)
Own:
- ambient effects
- inspect clues
- environmental state

### `gprog.c` (progression)
Own:
- XP
- leveling
- scaling

### `genc.c` (encounter)
Own:
- random encounter spawning
- bandit logic
- encounter sequencing

Goal:
- reduce coupling
- simplify debugging
- improve maintainability
- prepare for renderer/platform separation

---

## ANSI C89 cleanup and compiler enforcement

### Remove all K&R function definitions

✅ Done - no K&R definitions remain in `src/`.

### Enforce strict C89 compilation

✅ Done - flags live in the `Makefile`.

GCC:

```text
-std=c89
-pedantic
-Wall
-Wextra
-Wshadow
-Wstrict-prototypes
-Wmissing-prototypes
```

Tests/CI only:

```text
-Werror
```

Compiler discipline and warning cleanliness should remain early priorities throughout development.

### Ensure OpenWatcom parity

✅ Done - `make prepare-dos` covers the OpenWatcom path; ongoing discipline.

### Remove mixed declarations/statements

✅ Done - enforced by `-pedantic`.

### Replace C99 comments

✅ Done - no `//` comments remain in `src/`.

### Centralize gameplay tuning values

✅ Done - gameplay tuning values live in `include/config.h` (#33).

---

## #41 - Compatibility typedefs

✅ Done - [`include/base.h`](include/base.h) defines `u8`/`u16`/`u32` with documented width assumptions and compile-time `sizeof` guards; tick and byte-flag fields in `game.h` / `world.h` use these types.

---

# Phase 2 - State Ownership and Boundary Isolation

## #43 - Replace overlapping flags with state structures

Done ✅.

[`src/game.h`](src/game.h) defines `GameMode` (`GAME_MODE_EXPLORE`, `GAME_MODE_DIALOGUE`, `GAME_MODE_COMBAT`), `DialogueKind` (room NPCs including frog, wanderer, enemy), and `CombatState` (`enemy_hp`, `defending`). `GameState` holds `mode`, `dialogue`, and `combat` instead of overlapping `pond_dialogue`, `wanderer_dialogue`, `enemy_dialogue`, `npc_dialogue`, and `combat_active` flags. Transitions go through `game_set_mode_explore`, `game_set_mode_dialogue`, and `game_set_mode_combat` in [`src/game.c`](src/game.c).

---

## #44 - Formalize engine boundaries

Done ✅.

- Documented core / render / platform layers in `docs/architecture.md`
- Moved all `invent.c` player output to `render_inv_*` in `grendr`
- Added `make check-layers` to reject `printf` outside `main.c`, `grendr.c`, and the platform file (`platpos.c` / `platdos.c`) (run explicitly or via `make all-test`; not part of `make test`)

---

## #45 - Platform layer

Done ✅.

- [`include/platform.h`](include/platform.h) - `plat_poll_line`, `plat_time_now`, `plat_seed_rng`
- [`src/platdos.c`](src/platdos.c) / [`src/platpos.c`](src/platpos.c) - DOS vs POSIX implementations (FAT 8.3 names; flat `src/` layout)
- [`src/main.c`](src/main.c) - no `#ifdef __WATCOMC__`; orchestration only

---

# Phase 3 - Deterministic Test Harness Evolution

## Existing snapshot testing

Pattern:

```text
test.input
test.expect
```

Run:

```text
dosmud < test.input > test.output
diff test.output test.expect
```

---

## #66 / #112 - Improve deterministic test setup

Done ✅.

`TEST_MODE` builds link [`src/testharn.c`](src/testharn.c). Snapshot `.input` files can use `@fixture <name>` to reach known state without RNG-walking setup commands. Bandit fixtures call `game_reset_fixture_baseline` (shared with `game_init` mutable setup) before encounter-specific steps. Initial fixtures: `bandit_dialogue`, `bandit_handover_pick`, `bandit_wielded_pick` (see [`docs/testing.md`](docs/testing.md)). Bandit handover snapshot tests use fixtures instead of `take stick` + encounter rolls.

Follow-up (under [#40](https://github.com/ianmays/dosmud/issues/40) umbrella):

- **#112** (open) - migrate remaining snapshot tests to fixtures (`equipment`, `area_items`, `craft_wielded`, `map`, `smoke` / `seed_cli`)
- **#115** (open) - Phase A: new snapshot tests for gameplay gaps (after #112)
- **#95** (open) - Phase B: unit tests via [greatest](https://github.com/silentbicycle/greatest); high branch coverage on core modules (~90%+)
- **#116** (open) - Phase C: stress/soak (optional)
- **#113** (open) - wanderer snapshot fixtures
- **#114** (open) - custom world boot fixture for deterministic snapshots

---

## #40 - Gameplay test coverage (umbrella epic)

**[#40](https://github.com/ianmays/dosmud/issues/40)** tracks overall coverage; implementation is split across child issues (no single mega-PR).

| Issue | Role |
|-------|------|
| [#112](https://github.com/ianmays/dosmud/issues/112) | Prerequisite: migrate 5 brittle snapshots to fixtures |
| [#115](https://github.com/ianmays/dosmud/issues/115) | Phase A: new snapshot tests (combat, NPC, loot, eat, inspect, wait, etc.) |
| [#95](https://github.com/ianmays/dosmud/issues/95) | Phase B: greatest unit tests; **~90%+ branch coverage** on core modules |
| [#116](https://github.com/ianmays/dosmud/issues/116) | Phase C: stress/soak (10k ticks, combat loops; optional) |
| [#113](https://github.com/ianmays/dosmud/issues/113) | Wanderer snapshot fixtures |
| [#114](https://github.com/ianmays/dosmud/issues/114) | Custom world boot fixture |

**Sequencing:** #112 -> #115 (and #113/#114 as needed); #95 in parallel after #112; #116 when prioritized.

**Snapshot gaps (Phase A):** combat defend/salve/victory/loot, room NPC `talk`/`reply`, eat/use, inspect, wait.

**Unit scope (Phase B):** `command`, `invent`, `combat`, `game`, `genc`, `wanderer`, `dialogue`, `gatmos`, `world` (fixed graph), `gprog`, `items`, `testharn`. Out of scope: `grendr`, `txtres`, `main`, platform glue.

---

## #46 - Runtime `--seed`

Done ✅.

```text
dosmud --seed 1234
```

---

# Phase 4 - Workflow and Tooling Maturity

## #73 - Rules

✅ Done - codified: subsystem ownership, architecture boundaries, portability constraints, workflow discipline.

---

## #74 - Skills

Capture:
- workflow knowledge
- repo conventions
- deterministic testing workflows
- DOS/OpenWatcom constraints
- architecture expectations

---

# Phase 5 - Advanced Architecture

## #47 - Event queue architecture

Future direction:

```text
gameplay -> event queue -> renderer
```

Introduce:

```c
struct GameEvent
```

Example event types:

```text
EVENT_DAMAGE
EVENT_MOVE
EVENT_DIALOGUE
EVENT_ITEM
EVENT_NOISE
```

Potential benefits:
- replay systems
- logging
- renderer flexibility
- deterministic event capture
- improved testing
- save/load consistency

This is one of the most important architectural upgrades in the long-term roadmap.

---

## #16 - Save/load system

Create:

```text
save.c
save.h
```

The current fixed-array architecture is already well suited to serialization because the project favors:
- fixed arrays
- deterministic state
- explicit structs

Avoid:
- pointers
- heap ownership
- variable-sized runtime structures
- function pointers in persistent state

Initial binary serialization is acceptable.

---

# Phase 6 - Renderer and Content Expansion

## #48 - SDL renderer

Target structure:

```text
core/
render_text/
render_sdl/
platform/
```

SDL should own ONLY:
- rendering
- audio
- input
- timing

NOT:
- gameplay
- simulation
- combat logic
- world state

---

## Content expansion

Only after core architecture feels stable.

Includes:
- factions
- economy
- weather
- schedules
- quests
- procedural encounters
- larger worlds

---

# Important Final Guidance

Preferred architecture style:
- procedural
- explicit
- deterministic
- modular
- era appropriate

Do NOT turn this into:
- ECS
- component-heavy abstractions
- macro metaprogramming
- object-emulation frameworks
- service locator architectures
- modern enterprise-style architecture

Simple ANSI C scales surprisingly far when module boundaries stay disciplined.
