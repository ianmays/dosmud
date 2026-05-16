# dosmud

Minimal DOS-first MUD-like prototype in ANSI C.

## Disclaimer

This project is a personal learning and experimentation project provided "as is", without guarantees, warranties, or support of any kind.

While some care is taken to keep the codebase stable and documented, no guarantee is made regarding:
- correctness
- compatibility
- security
- fitness for any particular purpose

Use, modify, and distribute the project at your own risk.

The project is actively evolving and may change significantly over time.

## At a glance
\* this is a (modified) AI-generated image - purely intended to give a sense of what this project is all about before you move on, do **not** consider this a source of truth

<img src="dosmud.png" width="100%">

## Quick start

Native development build:

```sh
make build
./dosmud
```

Native layers/strict/test checks:

```sh
make check-layers
make test
make test-run
```

DOS/Open Watcom path:

```sh
make prepare-dos
make run-dos
```

See the [Makefile](Makefile) for all commands.

## Manual

Long-form project documentation lives in `/docs`:

- [Manual Index](docs/index.md)
- [Architecture](docs/architecture.md)
- [Testing](docs/testing.md)
- [Contributing](docs/contributor-guide.md)

The manual is deployed as [Github Pages](https://ianmays.github.io/dosmud/).

## Repository docs at root

- `CONTRIBUTING.md` points to contributor workflow guidance.
- `AGENTS.md` contains repository guidance for coding agents.
- `DEV_PLAN.md` outlines the current development roadmap, architectural priorities, and project rationale.

The [dosmud GitHub Project](https://github.com/users/ianmays/projects/1) board and [Issues](https://github.com/ianmays/dosmud/issues) list track active work, sequencing, and implementation details.
