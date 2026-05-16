# Contributor Guide

Thanks for contributing to dosmud.

## Development Principles

- Keep code ANSI C89 / ISO C90 compatible.
- Preserve compatibility with both GCC and OpenWatcom.
- **FAT 8.3 (8+3) names for `src/` sources and headers:** MS-DOS 5.0 through 6.22 (and our Open Watcom real-mode build) assume classic FAT volumes where the portable primary filename is still eight characters plus a three-letter extension. Longer basenames (for example `progression.h`) fail on those trees and in some DOS-hosted toolchains. Keep basenames at most eight characters (see `grendr.c`, `invent.h`, `gprog.c`, `gatmos.c`, `docs/architecture.md`). The DOS build uses `build.bat` (see `docs/testing.md`): gameplay objects are archived into `gameplay.lib` so the final `wcl` link fits COMMAND.COM line limits. When you add or remove a gameplay translation unit, update `Makefile`, `build.bat` compile lines, and the `wlib` line that builds `gameplay.lib`.
- Keep gameplay deterministic for identical seed + inputs.
- Prefer simple, explicit, procedural code over heavy abstractions.
- Avoid unrelated refactors in the same PR.
- Keep core gameplay free of `printf` and other terminal I/O; use `render_*` in `grendr` instead (`make test` runs `check-layers` to enforce this).

## Pull Requests Required

Changes should be merged through Pull Requests rather than direct pushes to `main`.

Recommended workflow:

- create a focused branch
- open a draft PR early
- link relevant GitHub Issues
- keep commits small and reviewable
- update documentation when behavior or workflows change

The repo’s GitHub project uses a **Status** field on issues: **In progress** while you implement (before the PR exists), **Review** once the draft PR is up, **Done** after merge. Details for agents: [AGENTS.md](../AGENTS.md).

## Local Validation Before Opening a PR

Run:

```sh
make build
make test
make test-run
```

If your changes affect build flow, DOS runtime behavior, or orchestration scripts:

```sh
make prepare-dos
make run-dos
make prepare-dos MODE=TEST_MODE
```

Recommended for broader tooling/build validation:

```sh
make all-build
make all-test
```

For detailed environment and workflow information, see `testing.md`.

## Commit Style

- Keep commit messages concise.
- Use bullet points rather than long prose if needed.
- Start commit messages with a lower-case character.
- Keep commits logically focused and easy to review.

## Documentation Ownership

- `README.md` = quick-start entrypoint.
- `/docs` = long-form canonical documentation.
- `architecture.md` = subsystem and design guidance.
- `testing.md` = deterministic testing and build workflow.
- Prefer linking across documents rather than duplicating long sections.
