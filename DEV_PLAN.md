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

Highest-priority architecture task.

`game.c` currently owns too many systems and should become orchestration-focused only.

Target structure:

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

### `atmosphere.c`
Own:
- ambient effects
- inspect clues
- environmental state

### `progression.c`
Own:
- XP
- leveling
- scaling

### `encounter.c`
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

Do not assume exact widths blindly.
Document assumptions clearly.

Goal:
- improve portability clarity
- centralize low-level assumptions

---

# Phase 2 - State Ownership and Boundary Isolation

## #43 - Replace overlapping flags with state structures

Introduce:

```c
enum GameMode {
    MODE_EXPLORE,
    MODE_DIALOGUE,
    MODE_COMBAT
};
```

Example:

```c
struct CombatState
{
    int active;
    int enemy_hp;
    int defending;
};
```

Goal:
- prevent invalid state combinations
- clarify transitions
- improve testability

---

## #44 - Formalize engine boundaries

Separate:
- gameplay simulation
- rendering
- platform integration

### Core
Pure gameplay logic.

NO:
- DOS APIs
- terminal APIs
- SDL APIs
- rendering concerns
- timing APIs

Core gameplay must never know DOS/Linux/SDL/terminal APIs exist.

### Render
Presentation only.

Current `grendr.*` separation is already a strong direction and should be preserved.

### Platform
Input/timing/system integration only.

Gameplay systems must never directly depend on platform or renderer concerns.

---

## #45 - Platform layer

Create:

```text
platform/
    platform.h
    plat_dos.c
    plat_posix.c
```

Goal:
- isolate portability concerns
- reduce platform leakage
- simplify future renderer work

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

## #66 - Improve deterministic test setup

Current tests can rely too heavily on trial-and-error RNG progression.

Long-term direction:
- deterministic fixture setup
- controlled world bootstrapping
- deterministic actor placement
- explicit scenario construction
- injected gameplay state where appropriate

Potential capabilities:

```text
spawn encounters directly
construct inventory state
place actors/items deterministically
create known room layouts
```

Goal:
- reduce brittle setup flows
- reduce AI/agent thrashing
- preserve deterministic behaviour

This should evolve as structured test harness functionality rather than hidden cheat/debug behaviour.

---

## #40 - Expand deterministic regression coverage

Add:
- gameplay edge cases
- combat coverage
- inventory coverage
- world simulation coverage

Stress-test examples:
- 10,000 tick simulations
- repeated combat loops
- inventory edge cases
- world generation loops

---

## #46 - Runtime `--seed`

Allow:

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

# Immediate Recommended Order

## Start now

1. split `game.c`
2. state ownership cleanup
3. engine boundary isolation
4. platform layer
5. compatibility typedefs
6. warning cleanup and compiler rigor

---

## Then

7. deterministic test setup evolution
8. regression coverage expansion
9. runtime `--seed`
10. workflow/rules/skills maturity

---

## Later

11. event queue architecture
12. save/load
13. SDL renderer
14. large-scale gameplay expansion

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
