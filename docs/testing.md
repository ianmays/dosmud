# Testing and Build Validation

This page is the canonical source for build/test command workflow.

## Native local checks

Run from project root:

```sh
make build
make test
make test-run
```

Purpose:

- `make build`: native GCC development build
- `make test`: strict deterministic compile path (`-Werror`, `-DTEST_MODE`)
- `make test-run`: scripted input regression pass (`tests/smoke.*`, `tests/bandit_handover.*`, `tests/area_items.*`, `tests/map.*`)

## DOS/Open Watcom validation path

Use PowerShell-driven DOS prep from Linux host shell:

```sh
make prepare-dos
```

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

1. Start program, confirm initial tick `[T:0]`.
2. Enter `help`; tick remains unchanged. Enter `help craft` (or another topic); tick remains unchanged and a single-topic line prints.
3. Enter invalid command (for example `xyz`); tick remains unchanged.
4. Enter `look`; tick remains unchanged.
5. Enter `wait`; tick increments by exactly 1.
6. Enter `look`, then `move <listed-direction>`; room changes and tick increments by 1.
7. Enter `quit`; process exits cleanly.
8. Bandit hand-over (optional): from a room where a bandit encounter triggers with at least one bag item, enter `2`, then `give <item>` matching something you carry; the bandit should leave and the item should be removed.
