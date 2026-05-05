# dosmud

Minimal DOS-first MUD-like prototype in ANSI C.

## Design rules

- Keep source compatible with conservative ANSI C/K&R-era compilers.
- Avoid dynamic allocation in early milestones.
- Use fixed-size buffers and compile-time caps from `include/config.h`.
- Keep world simulation deterministic per tick and event-safe in a text console.

## Build — local (development / sanity check)

Use the root `Makefile` on a Unix-like host (Linux, macOS, WSL, etc.) with **GCC** to compile and run a native binary for quick testing. This is **not** the shipped DOS build.

```sh
make
./dosmud
```

```sh
make clean   # remove ./dosmud
```

## Build — DOS (Open Watcom, Windows)

The release target is a **DOS** executable built with **Open Watcom** on Windows.

1. Open a shell where Watcom tools are on `PATH` (run `owsetenv.bat` once per session, or use the “Open Watcom Build Environment” shortcut if your install provides it).
2. From the project root, run:

```bat
build.bat
```

That invokes `wcl` with the same flags as in the batch file and writes `dosmud.exe` in the project directory.

## Manual verification script

1. Start program, note initial tick shown as `[T:0]`.
2. Enter `help` -> prints command help, tick remains unchanged.
3. Enter invalid command (example: `xyz`) -> error, tick unchanged.
4. Enter `look` -> room details, tick unchanged.
5. Enter `wait` -> tick increments by exactly 1.
6. Enter `look`, pick any listed exit direction, then `move <that-direction>` -> room changes, tick increments by 1.
7. Enter `quit` -> program exits.

## World (16 areas, procedural layout)

The game now uses a 16-room world (`CFG_ROOM_MAX`) with a procedurally linked layout on each run.
Room names are stable, but exact directional exits vary based on random linking.

Current room set:

- **Camp**
- **Road**
- **Pond**
- **Forest**
- **Ruins**
- **Stream**
- **Cliff**
- **Marsh**
- **Grove**
- **Bridge**
- **Catacombs**
- **Meadow**
- **Canyon**
- **Tower**
- **Orchard**
- **Cave**

Notes:

- A stable "spine" path is built first (including Camp, Road, Tower, Bridge, and selected wilds/ruins rooms), then the rest are attached to available exits.
- Pond and Orchard are forced to be east/west neighbors when both slots are available.
- In `TEST_MODE`, startup seed is fixed, so world generation is deterministic for repeatable tests.

Each area has a resident animal that periodically emits ambient noise.

The roaming adventurer keeps moving as time passes, even if you do not enter a command. If they enter your room, they proactively start a short chat (same reply keys as the frog). You will not get repeat bump dialogue until you are in a *different* room from them at least once.

## Current command set

- `look`
- `inspect [rustle|creak|water|grit]` for short-lived environmental clues that appear/disappear as ambient events happen
- `take <item>` / `drop <item>`
- `bag` (inventory, max 5 carried items)
- `eat <item>` (food items)
- `use <item>`
- `craft <torch|salve|spear>` from carried materials
- `loot` (during specific encounter outcomes)
- `move <north|south|east|west>`
- direction shorthand (`n`, `s`, `e`, `w`) also works
- `wait`
- `talk` (at the **Pond**: branching chat with the frog; answer with `1`/`2`/`3` or `reply <1-3>`)
- `1` / `2` / `3` or `reply <n>` when the **frog** or **traveler** is waiting for an answer
- `help`
- `quit`
