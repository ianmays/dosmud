# Testing and Build Validation

This page is the canonical source for build/test command workflow.

## Native local checks

Run from project root:

```sh
make build
make check-layers
make test
make test-run
```

Purpose:

- `make build`: native GCC development build
- `make check-layers`: core/render boundary guard (no `printf` in `src/*.c` except `main.c`, `grendr.c`, and the platform file `platpos.c` or `platdos.c`)
- `make test`: strict deterministic compile (`-Werror`, `-DTEST_MODE`, `-g -O0`); does not run `check-layers`
- `make test-run`: scripted input regression pass (`tests/smoke.*`, `tests/seed_cli.*`, `tests/bandit_handover.*`, `tests/bandit_wielded_give.*`, `tests/area_items.*`, `tests/map.*`, `tests/equipment.*`, `tests/craft_wielded.*`)

## Test fixtures (`TEST_MODE` only)

Snapshot tests can set up known game state without walking RNG-dependent commands. In a `make test` binary, lines in `.input` files of the form:

```text
@fixture <name>
```

are handled by `testharn` before normal command parsing. Fixture lines are not echoed as player commands. Unknown fixture names print `unknown test fixture` to stderr and exit with status 1. When a known fixture cannot finish setup, the binary prints `test fixture failed` to stderr and also exits with status 1.

Prefer fixtures over long setup scripts when a test needs a specific mode, inventory, or encounter. After changing fixture output, regenerate the matching `.expect` with `make test-run` and review the diff.

Bandit fixtures share a base reset first: explore mode, camp, tick 1, starting player stats (HP, level, XP, combat), empty bag, and no camp ground sticks, so later fixtures in the same run do not inherit prior damage or inventory.

| Fixture | State |
|---------|--------|
| `bandit_dialogue` | Base reset, stick in bag, bandit dialogue open |
| `bandit_handover_pick` | Base reset, stick in bag, bandit dialogue open, handover pick prompt (reply 2 already chosen) |
| `bandit_wielded_pick` | Base reset, stick wielded (`Atk:1`), bandit dialogue open, handover pick prompt |

Add new fixtures in [`src/testharn.c`](../src/testharn.c) and document them here. `testharn` is linked only for `make test` / `prepare-dos MODE=TEST_MODE`, not for `make build`.

## DOS/Open Watcom validation path

Use PowerShell-driven DOS prep from Linux host shell to build and sync the DOS tree:

```sh
make prepare-dos
```

Start DOS and launch the existing DOS executable without rebuilding or refreshing the tree:

```sh
make run-dos
```

`make run-dos` expects a previously prepared DOS tree. Run `make prepare-dos` first if the mirrored DOS files or executable are missing.

When you add or remove `src\*.c` files, update `Makefile` (`SRC` or `TEST_SRC`) and `build.bat`. For the Open Watcom path, keep every `wcl` and `wlib` line under the COMMAND.COM length limit (about 127 characters): gameplay sources are packed into `gameplay.lib` via several short `wlib` calls; the final `wcl` link lists `main.obj`, `platdos.obj`, `gameplay.lib`, plus the other `.obj` files. `TEST_MODE` compiles `testharn.c` to `tharn.obj` and appends it with a separate `wlib gameplay.lib +tharn.obj` line. Use `goto` labels in `build.bat` for conditionals; parenthesized `if (...)` blocks break under COMMAND.COM.

Deterministic DOS validation:

```sh
make prepare-dos MODE=TEST_MODE
```

Runtime seed (native or DOS build): the startup banner always prints the active seed, for example `dosmud (seed 1234)`. In `TEST_MODE` the default is `CFG_TEST_RAND_SEED` unless overridden on the command line:

```sh
./dosmud --seed 1234
```

Invalid flags print `usage: dosmud [--seed <unsigned>]` to stderr and exit with status 1. Seed values must be decimal, non-negative, and at most `CFG_SEED_CLI_MAX` (4294967295); leading `+`/`-` and out-of-range values are rejected.

## Combined cross-path checks

When changing build flow/tooling or other high-risk runtime behavior:

```sh
make all-build
make all-test
```

These targets intentionally exercise DOS prep/invocation and native GCC flow together.

## Environment and path model

- `make` runs from Linux shell.
- `prepare-dos.ps1` runs via Windows PowerShell.
- DOS emulator runs on Windows side.

In `prepare-dos.local.ps1`:

- `$source` should be Windows-reachable for Linux-hosted project files.
- `$mountpoint`, `$destination`, `$dospath` should be Windows-visible emulator paths.

The Open Watcom build (`build.bat`) only needs `src/`, `include/`, and `build.bat`. `prepare-dos.ps1` uses `robocopy /MIR` from `$source` with exclusions (not a whitelist): it skips `.git`, `tests/`, `docs/`, `.github/`, `.cursor/`, `.vscode/`, native build artifacts (`dosmud`, `*.output`, `*.o`, `*.obj`), and the Linux `Makefile`. Other top-level files (for example `README.md`) may still be copied.

## Build artifacts

- native path produces `./dosmud`
- DOS path produces `./dosmud.exe`
- DOS build transcript is `./build.log`

## Manual gameplay verification checklist

The tick HUD line includes `[Atk:n]`; `n` is the flat melee bonus used on combat attacks (level damage bonus plus wielded weapon). Expect the same value when editing snapshot `.expect` files after wield, unwield, or level-up.

1. Start program, confirm initial tick `[T:0]`.
2. Enter `help`; tick remains unchanged. Enter `help craft` (or another topic); tick remains unchanged and a single-topic line prints.
3. Enter invalid command (for example `xyz`); tick remains unchanged.
4. Enter `look`; tick remains unchanged.
5. Enter `wait`; tick increments by exactly 1.
6. Enter `look`, then `move <listed-direction>`; room changes and tick increments by 1.
7. Enter `quit`; process exits cleanly.
8. Bandit hand-over (optional): from a room where a bandit encounter triggers, enter `2`, then `give <item>` for something you carry in the bag **or** your wielded weapon (you can `wield` before replying `2` if the bag is empty); the bandit should leave and that item or wield slot should clear.
