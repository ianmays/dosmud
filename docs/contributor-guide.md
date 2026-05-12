# Contributor Guide

Thanks for contributing to dosmud.

## Development Principles

- Keep code ANSI C89 / ISO C90 compatible.
- Preserve compatibility with both GCC and OpenWatcom.
- Keep gameplay deterministic for identical seed + inputs.
- Prefer simple, explicit, procedural code over heavy abstractions.
- Avoid unrelated refactors in the same PR.

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
