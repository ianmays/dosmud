# DOSMUD Architecture Guide

## Overview

DOSMUD is a deterministic DOS-first adventure/RPG written in ANSI C89.

The project is designed around:
- simplicity
- portability
- deterministic simulation
- low memory overhead
- clear subsystem boundaries

The codebase intentionally avoids:
- dynamic allocation
- modern C dependencies
- heavy abstractions
- object-oriented patterns
- ECS architectures

The goal is to build a maintainable retro-style game engine that:
- runs under DOS/OpenWatcom
- builds cleanly with GCC
- supports deterministic testing
- can later support SDL rendering without rewriting gameplay systems

---

# Core Philosophy

## 1. Gameplay logic must be deterministic

Given:
- the same seed
- the same inputs

the simulation should behave identically.

This is extremely important for:
- testing
- debugging
- replay systems
- future save/load systems

Example:

```c
#ifdef TEST_MODE
    srand(1234);
#else
    srand(time(NULL));
#endif
```

The project avoids hidden randomness or frame-dependent simulation.

---

## 2. Gameplay code should not know about platforms

Core gameplay should never care whether the game is running:
- in DOS
- on Linux
- under SDL
- in a terminal

Platform-specific logic should remain isolated.

Good:

```c
game_process_input(game, line);
```

Bad:

```c
printf("Player moved.\n");
```

inside gameplay systems.

---

## 3. Fixed-size memory is preferred

The project intentionally uses:
- fixed arrays
- compile-time limits
- static storage

instead of heavy dynamic allocation.

Why:
- easier debugging
- deterministic behaviour
- DOS-friendly memory usage
- simpler save/load systems
- fewer ownership bugs

Example:

```c
int bag[CFG_BAG_MAX];
```

instead of:
- linked lists
- heap containers
- realloc-heavy systems

---

## 4. Simplicity beats abstraction

This project prefers:
- explicit code
- straightforward control flow
- procedural systems

over:
- generic frameworks
- macro metaprogramming
- complicated inheritance-style systems

The architecture should resemble:
- classic id Software
- Origin Systems
- Looking Glass
- early Blizzard

style engine design.

---

# Build Targets

This section is the authoritative build reference. Keep `README.md` focused on quick-start usage and keep detailed workflow behavior here.

## GCC/Linux Build

Purpose:
- fast iteration
- testing
- debugging

Example:

```sh
make build
./dosmud
```

This is NOT the primary shipping target.

---

## Makefile command contract

Primary targets:
- `make build` -> native GCC development build
- `make test` -> native GCC strict/test build (`-Werror`, `-DTEST_MODE`)
- `make test-run` -> runs `./dosmud < tests/input.txt > tests/output.txt`
- `make all-build` -> runs `clean`, then DOS prep/invocation, then native build
- `make all-test` -> runs `clean`, then DOS prep/invocation with `MODE=TEST_MODE`, then strict native build
- `make prepare-dos` -> PowerShell-based DOS preparation and launch flow only

Note:
- `all-build` and `all-test` intentionally verify both build paths in one deterministic command sequence.

---

## OpenWatcom DOS Build Pipeline

Purpose:
- real target platform
- compatibility validation
- parity checks against native flow

Primary entrypoint (from host shell):

```sh
make prepare-dos
```

Optional deterministic mode:

```sh
make prepare-dos MODE=TEST_MODE
```

Pipeline:
1. `prepare-dos.ps1` loads machine-local settings from `prepare-dos.local.ps1` (template: `prepare-dos.local.example.ps1`).
2. It mirrors project files into the configured DOS mount destination.
3. It starts the configured DOS executable and runs `build.bat` (optionally with `TEST_MODE`).
4. `build.bat` compiles each C file to `.obj`, links `dosmud.exe`, and writes `build.log`.

Environment/path model:
- run `make` from Linux, while `prepare-dos.ps1` and the emulator run from Windows.
- configure `$source` as a Windows-reachable path to the Linux project location.
- configure `$mountpoint`, `$destination`, and `$dospath` as Windows-side paths used by the emulator.

Direct in-environment fallback:

```bat
build.bat
```

Output expectations:
- native targets produce `dosmud`
- DOS target produces `dosmud.exe`
- DOS build transcript is stored in `build.log`

---

# High-Level Architecture

The engine currently follows this flow:

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

The important idea:
- commands mutate game state
- advancing time mutates the world
- rendering only displays state

---

# Main Subsystems

---

# `main.c`

## Responsibility

Owns:
- application startup
- main loop
- input polling
- timing
- top-level orchestration

It should NOT contain:
- gameplay rules
- combat logic
- world generation

---

## Main Loop

Simplified flow:

```text
poll input
    ->
process command
    ->
advance world tick
    ->
render state
```

---

## Important Concept: Background Ticks

The game advances simulation even while idle.

Example:
- wanderers move
- encounters can happen
- ambient events continue

This creates a living world instead of a turn-frozen one.

---

# `game.c`

## Responsibility

Currently acts as:
- central gameplay orchestrator

It coordinates:
- commands
- world updates
- encounters
- simulation flow

Over time, more systems should move out into dedicated modules.

---

## Important Functions

### `game_init()`

Initialises:
- world
- player
- inventory
- runtime state

This is the main setup function.

---

### `game_process_input()`

Core gameplay entry point.

Flow:

```text
raw text
    ->
command_parse()
    ->
apply_command()
    ->
advance_world_tick()
```

---

### `advance_world_tick()`

One of the most important functions in the project.

Responsible for:
- incrementing simulation time
- moving NPCs
- spawning encounters
- atmosphere events
- animal noises

This function is the "heartbeat" of the game.

---

# `command.c`

## Responsibility

Converts raw text into structured commands.

Example:

```text
move north
```

becomes:

```c
CMD_MOVE
dir = DIR_NORTH
```

---

## Important Principle

Parsing and execution are separate.

This is extremely important.

Bad architecture:

```text
parse + mutate state simultaneously
```

Good architecture:

```text
parse command
    ->
validate
    ->
apply command
```

This separation:
- simplifies testing
- improves debugging
- prevents partial mutations

---

## Time Advancement

Some commands advance simulation time:

```c
command_advances_time()
```

Examples:
- move
- wait
- take

Others do not:
- help
- look

This separation keeps simulation predictable.

---

# `world.c`

## Responsibility

Owns:
- room data
- procedural world generation
- movement validation
- room connectivity

---

## World Generation

The world is generated procedurally.

Flow:
1. Create room metadata
2. Build a stable "spine"
3. Attach additional rooms
4. Ensure connectivity

Important:
- room identities remain stable
- layout changes per seed

---

## Room Links

Rooms use directional exits:

```c
room->exits[DIR_NORTH]
```

Movement is graph-based rather than coordinate-grid based.

---

# `grendr.c`

## Responsibility

Rendering layer.

This subsystem should ONLY:
- display information
- print messages
- render UI

It should NEVER:
- modify gameplay state
- advance simulation
- contain gameplay rules

---

## Why This Separation Matters

This makes future SDL migration possible.

Eventually:

```text
Text renderer
SDL renderer
```

can both consume the same gameplay state.

---

## Good Pattern

Gameplay:

```c
render_msg_moved(dir_name);
```

Renderer:

```c
printf("You move north.\n");
```

Gameplay does NOT directly print text.

This is good architecture.

---

# `invent.c`

## Responsibility

Owns:
- bag management
- item usage
- crafting
- inventory mutations

---

## Important Pattern

Inventory logic stays inside inventory systems.

Avoid:

```c
game->bag_count--;
```

from unrelated systems.

Instead use:

```c
game_inv_bag_remove_item(...)
```

This centralises inventory rules.

---

# `items.c`

## Responsibility

Defines:
- item metadata
- item naming
- item lookup

This is effectively the game's item database.

---

# Core Data Structures

---

# `GameState`

Most important structure in the project.

Contains:
- world
- player state
- inventory
- dialogue state
- combat state
- runtime simulation state

The entire simulation lives here.

---

## Important Rule

Gameplay systems should mutate `GameState`.

Avoid:
- hidden globals
- duplicated state
- subsystem-local shadow copies

---

# `World`

Contains:
- rooms
- room count

Acts as the world graph.

---

# `Room`

Contains:
- metadata
- exits
- ambient creature data

Rooms are lightweight and static.

---

# Deterministic Simulation

## Why Determinism Matters

Deterministic behaviour allows:
- automated tests
- reproducible bugs
- stable balancing
- replay systems

---

## Important Rules

### Seed randomness once

Good:

```c
srand(seed);
```

at startup only.

Bad:

```c
srand(time(NULL));
```

inside gameplay systems.

---

### Simulation advances through ticks

All world evolution should happen through:
- commands
- background ticks

Never through rendering frequency.

---

# ANSI C89 Rules

This project targets:
- ANSI C89
- ISO C90

---

## Allowed

- prototype-style functions
- fixed arrays
- explicit structs
- procedural flow

---

## Avoid

### No C99 features

Do NOT use:
- `//` comments
- declarations inside `for`
- mixed declarations/statements
- `stdint.h`
- designated initialisers

---

## Avoid recursion

DOS stacks are limited.

Prefer iterative systems.

---

## Avoid heavy heap allocation

Use:
- static memory
- fixed buffers
- compile-time limits

unless dynamic allocation is truly necessary.

---

# How To Add Features

---

# Example: Adding a New Item

Suppose we want:
- `ITEM_ROPE`

---

## Step 1 — Add enum

In `items.h`:

```c
ITEM_ROPE
```

---

## Step 2 — Add metadata

In `items.c`:

```c
"rope"
```

---

## Step 3 — Add gameplay behaviour

Examples:
- crafting ingredient
- usable item
- quest item

Handled inside:
- inventory
- crafting
- gameplay systems

---

## Step 4 — Seed into world

Example:

```c
game->room_item[WORLD_ROOM_CAVE] = ITEM_ROPE;
```

---

# Example: Adding a New Command

Suppose:
- `sleep`

---

## Step 1 — Add command enum

In `command.h`:

```c
CMD_SLEEP
```

---

## Step 2 — Parse command

In `command.c`:

```c
if (strcmp(word1, "sleep") == 0)
```

---

## Step 3 — Add gameplay behaviour

In gameplay layer:

```c
if (cmd->type == CMD_SLEEP)
```

---

## Step 4 — Decide whether it advances time

Update:

```c
command_advances_time()
```

This is very important.

---

# Example: Adding an NPC

Suppose:
- hermit NPC

---

## Step 1 — Define room association

Example:

```c
if (room_id == WORLD_ROOM_CAVE)
```

---

## Step 2 — Add dialogue render functions

In renderer:
- intro text
- replies
- flavour

---

## Step 3 — Add dialogue state handling

Handled inside:
- dialogue systems
- reply processing

---

## Important Principle

NPCs should not directly manipulate unrelated systems.

Dialogue should:
- request actions
- not bypass ownership boundaries

---

# Example: Adding Random Events

Suppose:
- lightning storm

---

## Correct Pattern

Inside tick simulation:

```c
if ((rand() % 100) < chance)
```

inside:
- atmosphere systems
- world tick systems

---

## Incorrect Pattern

Do NOT tie events to:
- rendering
- frame rate
- input polling

---

# Common Mistakes To Avoid

---

## 1. Printing directly from gameplay logic

Bad:

```c
printf(...)
```

inside gameplay systems.

Use renderer functions instead.

---

## 2. Mutating unrelated subsystem state

Bad:

```c
game->bag_count--;
```

outside inventory systems.

---

## 3. Re-seeding RNG

Never:

```c
srand(...)
```

during gameplay.

---

## 4. Adding hidden globals

Prefer:

```c
struct GameState *game
```

passed explicitly.

---

## 5. Breaking determinism accidentally

Avoid:
- timing-dependent simulation
- random calls inside rendering
- simulation tied to FPS

---

# Future Architecture Direction

Planned long-term improvements:

- subsystem splitting
- state machine architecture
- event queue rendering
- save/load system
- SDL renderer
- deterministic regression testing

The project should evolve carefully without abandoning:
- simplicity
- explicit control flow
- deterministic behaviour
- ANSI C portability

---

# Final Guidance For Contributors

When adding features:
- prefer clarity over cleverness
- prefer explicit code over abstraction
- keep systems isolated
- keep gameplay deterministic
- maintain ANSI C89 compatibility

If unsure:
- choose the simpler design
- keep ownership boundaries clean
- avoid introducing invisible complexity

The project is intentionally old-school in architecture.
That is a strength, not a weakness.
