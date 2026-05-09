# Contributing

Thanks for contributing to dosmud.

## Development principles

- Keep code ANSI C89 / ISO C90 compatible.
- Keep gameplay deterministic for identical seed + inputs.
- Prefer simple, explicit, procedural code over heavy abstractions.
- Preserve compatibility with GCC and Open Watcom.

## Pull requests are required

Changes should be merged via Pull Request (no direct pushes to `main`) so CI can run build and basic test checks.

PR checklist:

- clear title and summary of why the change is needed
- local commands run and outcomes
- notes for DOS/Open Watcom validation when relevant

## Local validation before opening a PR

Run:

```sh
make build
make test
make test-run
```

If your changes affect build flow, startup/runtime pathing, or DOS behavior:

```sh
make prepare-dos
make prepare-dos MODE=TEST_MODE
```

Recommended for build/tooling changes:

```sh
make all-build
make all-test
```

For full command semantics and environment model, see [Testing](testing.md).

## Commit and scope hygiene

- Keep commits focused and reviewable.
- Avoid unrelated refactors in the same PR.
- Update docs when behavior, command flow, or setup expectations change.
- Commit messages should be concise and use bullet points rather than sentences if needed - first character should be lower-case

## Documentation ownership

- `README.md` is the quick-start entrypoint.
- `/docs` pages are the long-form canonical manual.
- Prefer links across pages rather than duplicating long command lists.
