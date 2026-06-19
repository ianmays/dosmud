# dosmud

#### Disclaimer

This project is a personal learning and experimentation project provided "as is", without guarantees, warranties, or support of any kind.

While some care is taken to keep the codebase stable and documented, no guarantee is made regarding:
- correctness
- compatibility
- security
- fitness for any particular purpose

Use, modify, and distribute the project at your own risk.

The project is actively evolving and may change significantly over time.

## Introduction

A DOS-first MUD-like prototype in ANSI C.

The [dosmud GitHub Project](https://github.com/users/ianmays/projects/1) tracks sequencing and active work (Status and **blocked-by** relationships on issues). [GitHub milestones](https://github.com/ianmays/dosmud/milestones) group roadmap themes; they are not a strict schedule. The [DEV_PLAN](DEV_PLAN.md) outlines the development roadmap, execution order for open work, and architectural priorities.

[CONTRIBUTING.md](CONTRIBUTING.md) points to contributor workflow guidance.  
[AGENTS.md](AGENTS.md) contains repository guidance for coding agents (workflow, documentation pass, Cursor rules and skills).

Long-form project documentation lives in `/docs`:

- [Manual Index](docs/index.md)
- [Architecture](docs/architecture.md)
- [Testing](docs/testing.md)
- [Contributing](docs/contributor-guide.md)

The manual is deployed as [Github Pages](https://ianmays.github.io/dosmud/).

The published [CI Metrics dashboard](https://ianmays.github.io/dosmud/ci-metrics.html) shows merged `main` CI history, including timing trends and soak benchmark history, for quick regression checks.

## At a glance
\* this is an AI-generated image - purely intended to give a sense of what this project is all about before you move on, do **not** consider this a source of truth as it may contain hallucinations 

<img src="dosmud.png" width="100%">

## Quick start

Native development build:

```sh
make build
./dosmud
./dosmud --version
./dosmud --seed 1234
```

Release and `TEST_MODE` runs both support in-session `save`, `load`, and `version` commands. The first pass uses a single-slot `save.dat` file in the current working directory.

Build identity comes from the checked-in [`VERSION`](VERSION) file plus generated metadata when Git is available. Native `make` builds write `build/include/version.h` with `+build.<count>.g<sha>[.dirty]`; DOS/OpenWatcom builds fall back to the checked-in [`include/version.h`](include/version.h) metadata if no generated header is present.

Replay logging is a `TEST_MODE` debugging path, not a release-build feature. Use it from a test build:

```sh
make test
./dosmud --seed 1234 --replay-log
./dosmud --seed 1234 --replay-log run.log
```

`--replay-log` writes a deterministic text log of each startup, input, and idle step plus the emitted `GameEvent` records for that step. With no path argument it defaults to `replay.log` in the current working directory. It does not change normal gameplay stdout output.

WSL cross-compile for a native Windows console executable:

```sh
sudo apt-get install -y mingw-w64
make build-win
make test-win
make win-run
```

This requires the MinGW cross-compiler (`x86_64-w64-mingw32-gcc`) in WSL and emits `dosmud.exe`. `make win-run` launches the existing executable in a new Windows console window and supports `SEED=<n>`; run `make build-win` or `make test-win` first.

Native layers/strict/test checks:

```sh
make check-layers
make test
make test-run
make test-unit
make test-soak
```

Full validation (snapshots, unit coverage, soak): `make test-all`.

Details: [Testing](docs/testing.md) — snapshots in `tests/regression/`, fixture harness in `tests/harness/`, unit tests in `tests/unit/`, soak/stress in `tests/soak/`.

DOS/Open Watcom path:

```sh
make dos-prepare
make dos-run
```

See the [Makefile](Makefile) for all commands.
