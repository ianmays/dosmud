# dosmud

Minimal DOS-first MUD-like prototype in ANSI C. The world now advances in
real time while waiting for input, and also advances on `move`/`wait`.

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
build_dos.bat
```

That invokes `wcl` with the same flags as in the batch file and writes `dosmud.exe` in the project directory.

## Manual verification script

1. Start program, note initial tick shown as `[T:0]`.
2. Enter `help` -> prints command help, tick remains unchanged.
3. Enter invalid command (example: `xyz`) -> error, tick unchanged.
4. Enter `look` -> room details, tick unchanged.
5. Enter `wait` -> tick increments by exactly 1.
6. Enter `move north` (from camp) -> room changes, tick increments by 1.
7. Enter `quit` -> program exits.

## World (12 areas)

Approximate map:

- **Camp** — north: Road, south: Meadow, east: Pond, west: Forest.
- **Road** — north: Cliff, south: Camp.
- **Pond** — west: Camp, east: Marsh.
- **Forest** — east: Camp, west: Grove, south: Stream.
- **Stream** — north: Forest, south: Ruins, east: Bridge.
- **Ruins** — north: Stream, south: Catacombs.
- **Cliff** — south: Road.
- **Marsh** — west: Pond.
- **Grove** — east: Forest.
- **Bridge** — west: Stream.
- **Catacombs** — north: Ruins.
- **Meadow** — north: Camp.

Each area has a resident animal that periodically emits ambient noise.

The roaming adventurer keeps moving as time passes, even if you do not enter a command. If they enter your room, they proactively start a short chat (same reply keys as the frog). You will not get repeat bump dialogue until you are in a *different* room from them at least once.

## Current command set

- `look`
- `move <north|south|east|west>`
- `wait`
- `talk` (at the **Pond**: branching chat with the frog; answer with `1`/`2`/`3` or `reply <1-3>`)
- `1` / `2` / `3` or `reply <n>` when the **frog** or **traveler** is waiting for an answer
- `help`
- `quit`
