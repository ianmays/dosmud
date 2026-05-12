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


## Quick start

Native development build:

```sh
make build
./dosmud
```

Local strict/test checks:

```sh
make test
make test-run
```

DOS/Open Watcom path:

```sh
make prepare-dos
```

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
- The GitHub Project board and Issues track active work, sequencing, and implementation details.
