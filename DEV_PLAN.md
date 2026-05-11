# DOSMUD ANSI C89 Development Plan

## Core Project Rules

The agent should treat these as non-negotiable:

```text
Language target:
- ANSI C89 / ISO C90
- OpenWatcom compatible
- GCC compatible
- No compiler extensions required
```

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

# Phase 1 — ANSI C89 Cleanup + Toolchain Enforcement

## Goal

Make the project strictly ANSI C89 clean and warning-clean.

---

## Tasks

### 1. Remove all K&R function definitions

✅ Done — no K&R definitions remain in `src/`.

Replace:

```c
func(a)
int a;
```

with:

```c
int func(int a)
```

Everywhere.

---

### 2. Enforce strict C89 compilation

✅ Done — flags live in the `Makefile`.

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

---

### 3. Ensure OpenWatcom parity

✅ Done — `make prepare-dos` covers the OpenWatcom path; ongoing discipline.

Goal:
- no GCC-only behaviour
- no implicit compiler extensions

Build both targets regularly.

---

### 4. Remove mixed declarations/statements

✅ Done — enforced by `-pedantic`.

Bad:

```c
foo();
int x;
```

Good:

```c
int x;
foo();
```

---

### 5. Replace C99 comments

✅ Done — no `//` comments remain in `src/`.

Remove:

```c
// comment
```

Use:

```c
/* comment */
```

---

### 6. Introduce compatibility typedefs

Tracking: #41.

Create:

```text
base.h
```

Example:

```c
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
```

Do NOT assume exact widths blindly.
Document assumptions.

---

### 7. Centralise constants

✅ Done — see #33; values live in `include/config.h`.

Move gameplay tuning values into config headers.

Example:

```c
#define CFG_BANDIT_SPAWN_CHANCE 14
#define CFG_ANIMAL_NOISE_CHANCE 75
```

---

# Phase 2 — Split `game.c`

Tracking: #42.

## Goal

Turn `game.c` into orchestration only.

Right now it owns too many systems.

---

## Target Structure

```text
src/
    game.c
    combat.c
    dialogue.c
    atmosphere.c
    wanderer.c
    progression.c
    encounter.c
```

---

## Responsibilities

### `game.c`
Only:
- high-level orchestration
- tick sequencing
- command routing

---

### `combat.c`
Own:
- combat state
- combat flow
- enemy turns
- combat rewards

---

### `dialogue.c`
Own:
- frog dialogue
- NPC dialogue
- reply handling

---

### `wanderer.c`
Own:
- wanderer movement
- encounter triggering
- separation logic

---

### `atmosphere.c`
Own:
- ambient effects
- inspect clues
- environmental state

---

### `progression.c`
Own:
- XP
- leveling
- scaling

---

### `encounter.c`
Own:
- random encounter spawning
- bandit logic

---

# Phase 3 — Replace Flag Soup with State Machines

Tracking: #43.

## Goal

Prevent invalid state combinations.

Current design uses many overlapping flags:
- `combat_active`
- `enemy_dialogue`
- `pond_dialogue`
- etc.

This will become unstable as systems grow.

---

## Introduce

```c
enum GameMode {
    MODE_EXPLORE,
    MODE_DIALOGUE,
    MODE_COMBAT
};
```

---

## Add subsystem state structs

Example:

```c
struct CombatState
{
    int active;
    int enemy_hp;
    int defending;
};
```

---

## Goal

Only one major mode active at a time.

---

# Phase 4 — Formalise Engine Boundaries

Tracking: #44.

## Goal

Separate:
- simulation
- rendering
- platform

cleanly.

---

## Introduce subsystem boundaries

### Core
Pure gameplay logic.

NO:
- printf
- DOS APIs
- SDL APIs
- timing APIs

---

### Render
Text output only.

Current `grendr.*` separation is already strong.

---

### Platform
Input/timing/system integration only.

---

# Phase 5 — Introduce Platform Layer

Tracking: #45.

## Goal

Prevent platform-specific leakage.

---

## Create

```text
platform/
    platform.h
    plat_dos.c
    plat_posix.c
```

---

## Wrap

- timing
- input polling
- sleep
- startup/shutdown
- randomness seed setup

---

## Main rule

Core gameplay must never know:
- DOS
- Linux
- SDL
- terminal APIs

exist.

---

# Phase 6 — Deterministic Test Harness

Tracking: #40 (general test improvements), #46 (runtime `--seed` argument).

## Goal

Turn deterministic simulation into real regression testing.

You already have the foundations.

---

## Add

```text
tests/
```

---

## Snapshot testing

Tracking: #66 (better test setup), #40 (more test coverage).

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

## Improve deterministic setup control

Current tests can rely too heavily on advancing RNG state through trial-and-error gameplay sequences to reach the desired scenario.

Long-term direction:
- deterministic fixture setup
- controlled world bootstrapping
- injected gameplay state for tests
- deterministic actor placement
- scenario-oriented harness helpers

Potential examples:

```text
spawn specific encounters
force inventory state
place actors/items directly
construct known world layouts
```

Goal:
- keep gameplay deterministic
- reduce fragile setup flows
- reduce AI/agent thrashing while discovering valid test states
- improve readability and intent of gameplay tests

This should evolve as structured test harness functionality rather than hidden cheat/debug behaviour.

---

## Add runtime seed argument

Example:

```text
dosmud --seed 1234
```

instead of relying entirely on compile-time test mode.

---

## Add simulation stress tests

Examples:
- 10,000 ticks
- repeated combat
- world generation loops
- inventory edge cases

---

# Phase 7 — Event Queue Architecture

Tracking: #47.

## Goal

Decouple simulation from rendering.

Current:

```text
game logic -> render function directly
```

Future:

```text
game logic -> event queue -> renderer
```

---

## Introduce

```c
struct GameEvent
```

Examples:

```text
EVENT_DAMAGE
EVENT_MOVE
EVENT_DIALOGUE
EVENT_ITEM
EVENT_NOISE
```

---

## Benefits

Enables:
- SDL rendering
- replay system
- logging
- testing
- save/load consistency

This is the biggest architectural upgrade in the entire roadmap.

---

# Phase 8 — Save/Load System

Tracking: #16.

## Goal

Stabilise persistent state ownership.

Because the project already uses:
- fixed arrays
- deterministic state
- explicit structs

serialization will be manageable.

---

## Add

```text
save.c
save.h
```

---

## Initial format

Simple binary serialization is fine initially.

Avoid:
- pointers
- heap ownership
- variable-sized runtime structures

---

# Phase 9 — SDL Renderer

Tracking: #48.

## Goal

Add graphics without damaging the core architecture.

---

## Final structure

```text
core/
render_text/
render_sdl/
platform/
```

---

## SDL owns ONLY

- rendering
- input
- timing
- audio

NOT:
- combat
- movement
- simulation
- world logic

---

# Phase 10 — Content Expansion

Only after architecture stabilises.

Then safely add:
- quests (#49)
- factions (#9)
- economy (#50)
- weather (#51)
- procedural encounters (#54)
- NPC schedules (#52)
- larger worlds (#55)
- persistence systems (#16)

without architectural collapse.

---

# Immediate Recommended Order

## Start now

1. ANSI cleanup
2. split `game.c`
3. constants cleanup
4. warning cleanup

---

## Then

5. platform layer
6. state machine conversion
7. deterministic test harness

---

## Later

8. event queue
9. save/load
10. SDL renderer

---

# Important Final Guidance For The Agent

style C architecture:
- procedural
- explicit
- deterministic
- modular
- simple data ownership

Do NOT turn this into:
- ECS
- component-heavy abstractions
- macro metaprogramming
- object-emulation frameworks
- modern enterprise-style architecture

Simple ANSI C scales surprisingly far when module boundaries stay disciplined.
