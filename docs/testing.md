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
- `make check-layers`: core/render boundary guard (no `printf` in `src/*.c` except `main.c` and `grendr.c`)
- `make test`: strict deterministic compile (`-Werror`, `-DTEST_MODE`, `-g -O0`); does not run `check-layers`
- `make test-run`: scripted input regression pass (`tests/smoke.*`, `tests/bandit_handover.*`, `tests/bandit_wielded_give.*`, `tests/area_items.*`, `tests/map.*`, `tests/equipment.*`, `tests/craft_wielded.*`

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

When you add or remove `src\*.c` files, update `Makefile` (`SRC`) and `build.bat`. For the Open Watcom path, keep the final `wcl` link line under the COMMAND.COM length limit (about 127 characters): gameplay sources are packed into `gameplay.lib` via `wlib` so the link line matches the pre-split shape (`main.obj`, `platdos.obj`, `gameplay.lib`, plus the other `.obj` files). Add new gameplay `.obj` names to the `wlib` line in `build.bat`; platform objects stay outside `gameplay.lib`.

Deterministic DOS validation:

```sh
make prepare-dos MODE=TEST_MODE
```

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
