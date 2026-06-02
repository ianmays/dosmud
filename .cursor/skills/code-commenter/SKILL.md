---
name: code-commenter
description: >-
  Add or review ANSI C89 comments in dosmud src/ and include/ for subsystem
  ownership, architecture seams, determinism, and transitional layers. Use when
  the user asks for a comment pass, after refactors that move ownership or
  seams, on PR-touched translation units, or to clarify intent without behavior
  changes. Comment-only by default.
---

# Code commenter

Comment specialist for dosmud (ANSI C89 / ISO C90, DOS/OpenWatcom, deterministic gameplay). Explain **why** and **who owns what**; do not restate obvious code.

**Procedure (this skill).** Scope from diff or user paths, match nearby style, edit comments in place, report using the output format below.

**Judgement (subagent).** Use [`.cursor/agents/code-commenter.md`](../../agents/code-commenter.md) when delegating a full pass: what to comment, what to skip, misread risk.

When applying edits: comment-only within scope; do not change executable behavior unless the user explicitly asks for code fixes.

## When to run

1. User names files or asks for a comment review on a PR/issue branch.
2. After refactors that change module ownership, event queues, or core/render boundaries.
3. Comment-only commits on an existing feature branch (no snapshot churn unless output changed).

## Scope

- Default: files in the current `git diff` vs base branch, or paths the user listed.
- Read one or two nearby modules first (`src/game.c`, `src/invent.c`, `src/gout.c`) for style, then edit in scope only.
- Do not run repo-wide comment sweeps unless the user asks.

## Repository conventions

| Rule | Detail |
|------|--------|
| Syntax | `/* ... */` only. Never `//` (C99; `-pedantic`). |
| Module header | Top of `.c`: 2-4 lines on subsystem ownership. |
| Inline | Sparse: invariants, seams, determinism, fixed buffers, transitional APIs, test harness quirks. |
| Tone | Concise, factual; phrase-style prose inside comments is fine. |
| Skip | Obvious loops, null checks, assignments unless **why** is non-obvious. |
| TODOs | Do not add unless they reference an existing issue number. |

Architecture context: [`docs/architecture.md`](../../../docs/architecture.md), especially core/render/platform boundaries and active seams such as `gout` / `GameEvent`.

## Comment priorities

1. **Subsystem ownership** - who mutates state; no platform/render leakage in core.
2. **Architecture seams** - e.g. core enqueues `GameEvent`, `grendr` drains and prints.
3. **Determinism** - RNG, tick order, queue overflow, fixed-size limits.
4. **Transitional scaffolding** - mark temporary paths; cite a follow-up issue only if it is already filed (e.g. `/* Temporary legacy output path until #157 migrates remaining command events. */`).
5. **Non-obvious control flow** - mode gates, handover state, compact slot removal, quiet-tick test behavior.

## Do not comment

- Obvious syntax or names.
- Mechanical refactors with no architectural change (unless asked).
- `txtres` copy or snapshot strings (unless parsing/layout logic).
- Files outside requested or PR scope.

## Technical constraints

- C89/C90; comments must not imply C99+ language features.
- FAT 8.3 basenames under `src/`; no rename suggestions in a comment-only pass.
- Point to `grendr` / `plat*` for I/O boundaries; do not add `printf` in core modules.

## Workflow

1. `git diff` (or user paths) for scope.
2. Per touched `.c` / `.h`:
   - missing module header?
   - new public API or seam without a boundary note?
   - legacy vs new path without transitional note?
3. Edit comments in place; no drive-by reformatting.
4. Headers: brief block comment when contracts are non-obvious (see `src/gout.h`).

## Output format

```markdown
### Files updated
- ...

### Comments added
- `path`: what boundary/invariant was documented

### Skipped
- `path` or area: one-line reason

### Risks
- only if comments may drift soon (e.g. active migration)
```

Stay minimal: one good comment at a seam beats ten on obvious lines.

## Out of scope unless asked

- Logic changes, renames, broad sweeps.
- `DEV_PLAN.md`, GitHub issues, PR bodies.
- New tests for comment-only work.
