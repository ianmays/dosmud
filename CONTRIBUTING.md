# Contributing to dosmud

Thanks for contributing.

## Development Principles

- Keep code ANSI C89 / ISO C90 compatible.
- Keep gameplay deterministic for identical seed + inputs.
- Prefer simple, explicit, procedural code over heavy abstractions.
- Preserve compatibility with both GCC and Open Watcom.

## Local Build and Test Expectations

Before opening a PR, run local checks from the project root:

```sh
make build
make test
make test-run
```

If your change can affect DOS behavior, toolchain flow, startup, or docs for build/setup, also validate the DOS path locally:

```sh
make prepare-dos
```

For deterministic DOS validation:

```sh
make prepare-dos MODE=TEST_MODE
```

Recommended when touching build logic:

```sh
make all-build
make all-test
```

## Pull Requests Are Required

Changes should be merged via Pull Request (no direct pushes to main), so CI can run the build and basic test checks.

PR checklist:
- clear title and summary of why the change is needed
- local commands run and outcomes
- notes for any DOS/Open Watcom validation performed

## Commit and Scope Hygiene

- Keep commits focused and reviewable.
- Avoid unrelated refactors in the same PR.
- Update docs when command flows, setup, or behavior changes.

## Build Documentation Ownership

- Keep quick-start usage in `README.md`.
- Keep detailed build/pipeline behavior in `PROJECT_GUIDE.md`.
- Link between docs rather than duplicating long sections.
