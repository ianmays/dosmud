# AGENTS.md

Guidance for AI/code agents working in this repository.

## IMPORTANT - Read these non-negotiables first

- Always create a branch before making changes.
- Always check for needed documentation updates/additions/removals when making changes - ensure documentation style remains consistent (e.g. if everything else is listed as bullet-points - then use bullet points).
- Commit messages should be concise and use bullet points rather than sentences if needed - first character should be lower-case
- Always raise draft PRs
- If you have already opened a PR, include pushing the next set of updates to the branch in any Plan.
- If picking-up an Issue from the Dosmud project to work on - manage the status in GitHub, likewise always check if an Issue exists for what you're working on - ensure all relevant issues are linked to PRs

## Purpose

- Keep changes aligned with DOS-first goals and deterministic ANSI C design.
- Preserve fast local iteration on Linux while validating DOS/Open Watcom compatibility.
- Avoid architectural drift and documentation duplication.

## Non-Negotiable Technical Constraints

- Target language: ANSI C89 / ISO C90.
- Must remain compatible with both GCC and Open Watcom.
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

- Native Linux build loop:
  - `make build`
  - `make test`
  - `make test-run`
- Cross-path validation when touching build/runtime behavior:
  - `make all-build`
  - `make all-test`
- DOS flow entrypoint:
  - `make prepare-dos` (uses `prepare-dos.ps1`)
  - optional deterministic mode: `make prepare-dos MODE=TEST_MODE`

## Environment Model for DOS Flow

- `make` is executed from Linux.
- PowerShell tooling (`prepare-dos.ps1`) executes via Windows PowerShell.
- DOS emulator is launched on the Windows side.
- In `prepare-dos.local.ps1`:
  - `$source` should be a Windows-reachable path to Linux project files.
  - `$mountpoint`, `$destination`, `$dospath` should be Windows-side paths visible to the emulator.

## Documentation Ownership (Do Not Duplicate)

- `README.md` = quick-start/operator usage:
  - common commands
  - minimal setup steps
  - high-level artifact outcomes
- `/docs/index.md` = manual entrypoint
- `/docs/architecture.md` = architecture rationale and subsystem guidance
- `/docs/testing.md` = build/test command contract and environment model
- `/docs/contributor-guide.md` = contribution process and PR expectations
- When adding docs, link between files instead of repeating long sections.

## Editing and Change Hygiene

- Make focused changes; do not refactor unrelated areas opportunistically.
- Preserve existing naming, file layout, and style unless asked otherwise.
- If you modify build docs or scripts, verify wording against:
  - `Makefile`
  - `prepare-dos.ps1`
  - `build.bat`
- Prefer small, reviewable commits with clear intent.

## When in Doubt

- Choose the simpler implementation.
- Favor deterministic behavior and explicit ownership boundaries.
- Keep DOS/Open Watcom compatibility ahead of convenience features.
