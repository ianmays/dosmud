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

`game` should not grow into a monolith; specialized systems should move into dedicated modules over time.

## `command`

- parse raw text into structured commands
- keep parsing separate from execution/mutation

## `world`

- room graph data and connectivity
- procedural world generation
- movement validation

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
