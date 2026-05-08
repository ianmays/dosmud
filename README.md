# dosmud

Minimal DOS-first MUD-like prototype in ANSI C.

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
- `DEV_PLAN.md` keeps track of upcoming development phases. 