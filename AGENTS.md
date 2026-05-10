# AGENTS.md

Guidance for AI/code agents working in this repository.

## IMPORTANT - Read these non-negotiables first

- Always create a branch before making changes.
- Always raise draft PRs.
- Always check for needed documentation updates/additions/removals when making changes.
- Keep documentation style consistent with surrounding files.
- Commit messages should be concise; use bullet points rather than sentences if needed; first character should be lower-case.
- Always check if a corresponding GitHub Issue exists before starting work (see: dosmud GitHub project).
- Ensure all relevant Issues are linked to PRs.
- Manage issue status in GitHub - ensure Issue status and Issue project status are aligned
- If no Issue exists, create one and label it appropriately (`gameplay`, `non-functional`, `tooling`, etc)
- Always add the `agent` label to any issue you create.
- Always add Issues to the dosmud Github project
- If a PR already exists, include pushing follow-up updates to the branch in any implementation plan.
- When asked to pick up a new issue (possibly under a specific label), ensure you pick the top issue according to the dosmud Github project - you should only pull from the tickets that exist in the 'Agent-ready' column

### GitHub project `Status` column (dosmud project)

Keep each issue’s card aligned with real progress. Do not skip **In progress** on the way to **Review**:

- **In progress** — set as soon as work on the issue begins (e.g. branch created or first commit), and keep it there until the linked PR is opened.
- **Review** — set when you open the draft PR that links the issue (first time the work is on the board as a PR).
- **Done** — set only **after** that PR has merged (and close the issue when the work is finished).

## Purpose

- Keep changes aligned with DOS-first goals and deterministic ANSI C design.
- Preserve fast local iteration on Linux while validating DOS/OpenWatcom compatibility.
- Avoid architectural drift and documentation duplication.

## Non-Negotiable Technical Constraints

- Target language: ANSI C89 / ISO C90.
- Must remain compatible with both GCC and OpenWatcom.
- Prefer fixed-size arrays and static storage.
- Avoid dynamic allocation unless explicitly justified.
- Avoid recursion.
- Avoid compiler-specific extensions and C99/C11 features.
- Keep state explicit; avoid hidden globals and cross-module state mutation.

## Architecture Guardrails

- Keep gameplay deterministic for identical seed + inputs.
- Keep simulation, rendering, and platform concerns separated.
- Keep `game.c` orchestration-focused; avoid re-centralizing unrelated systems.
- Prefer simple, procedural, explicit control flow over heavy abstraction patterns.
- Do not introduce ECS-like frameworks or object-emulation architectures.

## Build and Validation Workflow

Primary local validation loop:

- `make build`
- `make test`
- `make test-run`

Cross-path validation when touching build/runtime behavior:

- `make all-build`
- `make all-test`

DOS flow entrypoint:

- `make prepare-dos`
- deterministic DOS mode: `make prepare-dos MODE=TEST_MODE`

## Environment Model for DOS Flow

- `make` executes from Linux/WSL.
- `prepare-dos.ps1` executes through Windows PowerShell.
- DOSBox-X launches on the Windows side.
- `prepare-dos.local.ps1` stores machine-specific Windows/WSL path configuration.
- Do not assume the repository lives under `/mnt/c`.

## Documentation Ownership

- `README.md` = quick-start/operator usage.
- `/docs/index.md` = documentation entrypoint.
- `/docs/architecture.md` = subsystem boundaries and architecture rationale.
- `/docs/testing.md` = build/test workflow and deterministic testing model.
- `/docs/contributor-guide.md` = contributor workflow and PR expectations.
- Prefer links between documents instead of duplicating long instructions.

## Editing and Change Hygiene

- Make focused changes; avoid unrelated opportunistic refactors.
- Preserve existing naming, file layout, and style unless explicitly asked otherwise.
- Verify build/tooling documentation against actual scripts and Make targets.
- Prefer small, reviewable commits with clear intent.

## Cursor Cloud Instructions

- Expected environment: Ubuntu-based Linux VM.
- Required dependencies: `gcc` and `make`.
- Full supported loop:
  - `make build`
  - `make test`
  - `make test-run`
- `make test` builds with `-Werror -DTEST_MODE`.
- `make test-run` pipes scripted input and diffs against snapshot output.
- `make prepare-dos` and DOS-path validation targets are unavailable in cloud VMs.
- No lint tooling exists beyond GCC warning enforcement.

## When in Doubt

- Choose the simpler implementation.
- Favor deterministic behavior and explicit ownership boundaries.
- Prioritize DOS/OpenWatcom compatibility over convenience features.
