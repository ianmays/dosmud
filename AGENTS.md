# AGENTS.md

Guidance for AI/code agents working in this repository.

## Purpose

- Keep changes aligned with DOS-first goals and deterministic ANSI C design.
- Preserve fast local iteration on Linux while validating DOS/OpenWatcom compatibility.
- Avoid architectural drift and documentation duplication.

## Core Agent Rules (Non-Negotiable)

- ALWAYS start in plan mode - then IMMEDIATELY move the linked Issue to **Planning** (same response, before issue research or plan drafting)
- ALWAYS check for an existing GitHub Issue before starting work.
- NEVER modify or reopen Closed Issues.
- ALWAYS create a branch before making changes.
- ALWAYS include mark work as done in  `DEV_PLAN.md` when raising a PR.
- ALWAYS open draft PRs first.
- ALWAYS link PRs to their corresponding Issues.
- ALWAYS check whether documentation updates are required.
- ALWAYS preserve deterministic gameplay behavior.
- NEVER introduce gameplay/render/platform coupling.
- NEVER use em dash - use standard hyphen only.
- NEVER introduce C99/C11 features or compiler-specific extensions.

## GitHub Workflow (dosmud project)

### Task start checklist

1. SwitchMode -> plan
2. Select top Agent-ready issue (or confirm user-specified issue)
3. Move issue to **Planning** on project board (same turn as step 1)
4. Research and draft plan
5. Post plan comment on issue
6. Wait for user approval
7. Move to **In progress** and create branch

Plan mode forbids code and repo changes, but GitHub project status updates and issue comments are always allowed.

### Issue selection

- When asked to pick up a new issue, ALWAYS select the top issue from the `Agent-ready` column in the dosmud GitHub project.
- NEVER begin work on an issue you believe should be abandoned, deferred, or reconsidered - challenge the work instead.

### Issue creation

- If no Issue exists, ALWAYS create one.
- ALWAYS apply appropriate labels (`gameplay`, `non-functional`, `tooling`, `documentation`, etc).
- ALWAYS add the `agent` label to agent-created Issues.
- ALWAYS add new Issues to the dosmud GitHub project.

### Status transitions

Do not skip workflow stages.

- **Planning** - set in the same turn as `SwitchMode`, before issue research or plan drafting. Never defer until plan approval.
- **In progress** - set before implementation work starts.
- **Review** - set when the draft PR is opened.
- **Done** - set only after merge and Issue closure.

### Planning expectations

- ALWAYS add agreed implementation details as an Issue comment before moving to `In progress`.
- If a PR already exists, ALWAYS include updating the PR in the implementation plan.

### PR expectations

- ALWAYS raise draft PRs initially.
- NEVER include issue references in PR titles.
- ALWAYS ensure PR labels match Issue labels.
- ALWAYS keep PR titles and commit messages concise.
- ALWAYS use lower-case first character in PR titles and commit messages.
- NEVER use prefixes like `docs:` in PR titles or commit messages.

### Re-review workflow

- If pushing follow-up commits to a non-draft PR:
  - ALWAYS leave the comment `review this`
  - NEVER ask whether a re-review is needed

### Completion workflow

After merge:
- switch back to `main`
- pull latest changes
- delete completed local branches

## Technical Constraints

- Target language: ANSI C89 / ISO C90.
- Must remain compatible with both GCC and OpenWatcom.
- Prefer fixed-size arrays and static storage.
- Avoid dynamic allocation unless explicitly justified.
- Avoid recursion.
- Keep state explicit and localized.
- Avoid hidden globals and cross-module state mutation.

## Architecture Principles

- Keep gameplay deterministic for identical seed + inputs.
- Keep simulation, rendering, and platform concerns separated.
- Keep `game.c` orchestration-focused.
- Avoid re-centralizing unrelated systems into `game.c`.
- Prefer simple, procedural, explicit control flow.
- Avoid heavy abstraction patterns.
- Do not introduce ECS-like frameworks or object-emulation architectures.
- Favor explicit ownership boundaries over shared mutable systems.

## Validation Workflow

Primary local validation loop:

```sh
make build
make check-layers
make test
make test-run
```

Cross-path validation when touching build/runtime behavior:

```sh
make build-all
make test-all
```

DOS workflow:

```sh
make dos-prepare
make dos-run
```

Deterministic DOS mode:

```sh
make dos-prepare MODE=TEST_MODE
```

## Environment Model for DOS Flow

- `make` executes from Linux/WSL.
- `dos-prepare.ps1` executes through Windows PowerShell.
- DOSBox-X launches on the Windows side.
- `dos-prepare.local.ps1` stores machine-specific path configuration.
- Do not assume the repository exists under `/mnt/c`.

## Documentation Ownership

- `README.md` = quick-start/operator usage
- `docs/index.md` = documentation entrypoint
- `docs/architecture.md` = subsystem boundaries and rationale
- `docs/testing.md` = deterministic testing workflow
- `docs/contributor-guide.md` = contributor and PR workflow

Prefer linking between documents over duplicating large instruction blocks.

## Editing and Change Hygiene

- Make focused changes.
- Avoid unrelated opportunistic refactors.
- Preserve surrounding naming/style/layout conventions unless explicitly asked.
- Verify build and tooling documentation against real scripts and targets.
- Prefer small, reviewable commits with clear intent.
- ALWAYS add tests for new gameplay features - aim for maximum coverage

## Cursor Cloud Instructions

Expected environment:
- Ubuntu-based Linux VM
- `gcc`
- `make`

Supported validation loop:

```sh
make build
make check-layers
make test
make test-run
```

Additional notes:
- `make check-layers` rejects `printf` in core gameplay files.
- `make test` builds with `-Werror -DTEST_MODE`.
- `make test-run` executes deterministic snapshot tests.
- DOS validation targets are unavailable in cloud VMs.
- No lint tooling exists beyond GCC warning enforcement.
- NEVER include co-author statements in commits.

## When in Doubt

- Choose the simpler implementation.
- Favor deterministic behavior.
- Favor explicit ownership boundaries.
- Prioritize DOS/OpenWatcom compatibility over convenience features.
- Prefer consistency with surrounding code over abstract "best practice".