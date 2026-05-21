# Contributor Guide

Thanks for contributing to dosmud.

## Development Principles

- Keep code ANSI C89 / ISO C90 compatible.
- Preserve compatibility with both GCC and OpenWatcom.
- **FAT 8.3 (8+3) names for `src/` sources and headers:** MS-DOS 5.0 through 6.22 (and our Open Watcom real-mode build) assume classic FAT volumes where the portable primary filename is still eight characters plus a three-letter extension. Longer basenames (for example `progression.h`) fail on those trees and in some DOS-hosted toolchains. Keep basenames at most eight characters (see `grendr.c`, `invent.h`, `gprog.c`, `platpos.c`, `docs/architecture.md`). The DOS build uses `build.bat` (see `docs/testing.md`): gameplay objects are archived into `gameplay.lib` via several short `wlib` lines so each COMMAND.COM invocation stays under the length limit; `TEST_MODE` adds `thwld.obj` and `tharn.obj` from `harness\` in a separate `wlib` pass (`dos-prepare` copies `tests\harness\`). When you add or remove a gameplay translation unit, update `Makefile`, `build.bat` compile lines, and the `wlib` calls that build `gameplay.lib`. Platform sources (`platdos.c` on DOS, `platpos.c` on GCC) link beside `main.obj`, not inside `gameplay.lib`.
- Keep gameplay deterministic for identical seed + inputs.
- Prefer simple, explicit, procedural code over heavy abstractions.
- Avoid unrelated refactors in the same PR.
- Keep core gameplay free of `printf` and other terminal I/O; use `render_*` in `grendr` instead (`make check-layers` allows `printf` only in `main.c`, `grendr.c`, and the platform file `platpos.c` or `platdos.c`; `make test-all` runs the guard before the test build). Newline and spacing rules for `txtres` and `grendr` are in [architecture.md](architecture.md#newline-and-spacing).

## Pull Requests Required

Changes should be merged through Pull Requests rather than direct pushes to `main`.

Recommended workflow:

- create a focused branch
- open a draft PR early
- link relevant GitHub Issues
- keep commits small and reviewable
- update documentation when behavior or workflows change

The repo’s GitHub project uses a **Status** field on issues: **Planning** when forming an implementation plan (add decided plan as a comment), **In progress** while you implement (before the PR exists), **Review** once the draft PR is up, **Done** after merge. Details for agents: [AGENTS.md](../AGENTS.md).

[`DEV_PLAN.md`](../DEV_PLAN.md) is a roadmap log: mark the issue **Done ✅** when you open a draft PR. It is not a living status tracker (no updates on push or merge).

### After you push

- While the PR is a **draft** on GitHub: do not post `review this`.
- After you mark the PR **Ready for review** (non-draft): comment `review this` on the PR after **each** push that should re-trigger review. Use that exact body text only.
- Project board **Review** can be set when the draft PR is opened; GitHub **Ready for review** (`isDraft` false) is what triggers the `review this` convention, not board status alone.

Agents automate this with `gh pr view --json number,isDraft` after every push; see [AGENTS.md](../AGENTS.md) and [`.cursor/skills/pr-after-push/SKILL.md`](../.cursor/skills/pr-after-push/SKILL.md).

## Local Validation Before Opening a PR

Run:

```sh
make build
make check-layers
make test
make test-run
make test-unit
```

Snapshot pairs live under `tests/regression/`; unit sources under `tests/unit/` (binary and coverage output in `tests/unit/build/`, gitignored). For snapshot fixtures, roll inject, unit tests, and `quiet_explore` tick tests, see [testing.md](testing.md).

If your changes affect build flow, DOS runtime behavior, or orchestration scripts:

```sh
make dos-prepare
make dos-run
make dos-prepare MODE=TEST_MODE
```

Recommended for broader tooling/build validation:

```sh
make build-all
make test-all
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
