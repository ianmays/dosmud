---
name: pre-draft-pr-passes
description: >-
  Default orchestrator for pre-draft-PR quality gates on behavioral branches:
  make test*, then test-auditor, code-commenter, and docs-steward subagents
  in order. Use after behavioral implementation before opening a draft PR,
  when the user asks to run pre-PR gates, or to catch up if passes were
  skipped. Skips steps already completed on the same diff this session.
disable-model-invocation: true
---

# Pre-draft PR passes

**Orchestrator (this skill).** Run `make test*`, then delegate test-gap, comment, and documentation passes to subagents in canonical order.

**Policy (canonical).** [`AGENTS.md`](../../../AGENTS.md) **Pre-draft PR passes** and per-pass sections; gate rules [testing-gap-after-implement](../../rules/testing-gap-after-implement.mdc), [code-commenter-after-implement](../../rules/code-commenter-after-implement.mdc), [documentation-after-implement](../../rules/documentation-after-implement.mdc).

**Per-pass procedure.** Child skills remain authoritative when running a single pass inline: [testing-gap-auditor](../testing-gap-auditor/SKILL.md), [code-commenter](../code-commenter/SKILL.md), [documentation-maintainer](../documentation-maintainer/SKILL.md).

## Branch diff scope

Match [`scripts/check-test-gaps.sh`](../../../scripts/check-test-gaps.sh): union **committed branch diff** and **uncommitted working tree vs HEAD**. Do not scope subagents on `origin/main...HEAD` alone - that misses edits not yet committed.

```sh
BASE="${BASE_REF:-origin/main}"
MERGE_BASE=$(git merge-base "$BASE" HEAD)
COMMITTED=$(git diff --name-only "${MERGE_BASE}...HEAD")
UNCOMMITTED=$(git diff --name-only HEAD)
SCOPE_FILES=$(printf '%s\n%s' "$COMMITTED" "$UNCOMMITTED" | sort -u | grep -v '^$' || true)
```

Parent computes `SCOPE_FILES` before Steps 1-3. Use it for skip checks, dedup fingerprints, and subagent prompts. Recompute after any subagent commit or before opening draft PR.

Recommend committing implementation on the feature branch before draft PR; passes may still run while the worktree has uncommitted edits.

## When to run

1. **Default:** end of behavioral implementation on a feature branch, **before** opening a draft PR.
2. User asks to "run pre-PR gates", "pre-draft passes", or similar.
3. **Catch-up:** parent skipped one or more passes before draft PR; re-run with skip logic below.
4. **Non-behavioral branches:** docs-only or tooling-only PRs - run step 3 (docs-steward) at minimum; steps 1-2 per skip rules (`Makefile` in `SCOPE_FILES` still triggers step 1).

**Not** the normal path after draft PR is open (that is [pr-after-push](../pr-after-push/SKILL.md)). Re-run only when the branch diff changed after the last pass or the user explicitly requests audit.

## When to skip entire composite

- All applicable passes already reported complete this session on current `SCOPE_FILES` (committed + uncommitted vs base).
- User opted out of pre-PR passes.

**Tooling-only or docs-only branches:** do not skip the entire composite. Skip steps 1-2 per their per-step rules when `SCOPE_FILES` has no test-gap or comment obligations. Still run step 1 when `Makefile` is in `SCOPE_FILES` (test list or coverage metadata may have changed). Still run step 3 (docs-steward) when the diff touches documentation signals (`.cursor/` rules/skills/agents, `docs/`, `AGENTS.md`, `README.md`, `DEV_PLAN.md`, contributor workflow).

## Checklist

```text
Pre-draft PR passes:
- [ ] 0. make test, make test-run, make test-unit (or make test-all) - stop on failure
- [ ] 0b. compute SCOPE_FILES (committed branch diff union git diff HEAD)
- [ ] 1. test-auditor subagent (or skip)
- [ ] 2. code-commenter subagent (or skip)
- [ ] 3. docs-steward subagent (or skip)
- [ ] 4. Combined output summary
```

## Step 0: make test* (parent agent)

```sh
make test
make test-run
make test-unit
```

Or `make test-all` when touching build/runtime paths. Do not delegate this step. Stop and fix failures before Step 1.

## Step 1: test-auditor

**Skip when:** no `src/`, `include/`, `tests/`, or `Makefile` paths in `SCOPE_FILES` (Makefile-only branches may change `SNAPSHOT_TESTS`, `UNIT_TEST_SRC`, or `COVERAGE_MODULES` per `check-test-gaps.sh`); user opted out; scope unchanged since a completed test-gap pass this session (see dedup guard).

**Delegate** via `Task` with `subagent_type: test-auditor`:

```markdown
Scope: SCOPE_FILES = committed branch diff (${MERGE_BASE}...HEAD) union git diff --name-only HEAD (see pre-draft-pr-passes skill Branch diff scope). Base ref: origin/main unless user named another.
Follow .cursor/skills/testing-gap-auditor/SKILL.md and .cursor/agents/test-auditor.md.
Run sh scripts/check-test-gaps.sh origin/main without TEST_GAP_INFORMATIVE; must exit 0 (script already unions committed + uncommitted).
May add or update tests; re-run make test, make test-run, make test-unit after changes.
Report using the testing-gap-auditor skill output format.
```

## Step 2: code-commenter

**Skip when:** no `src/`, `include/`, or `tests/` C sources in `SCOPE_FILES`; user opted out; scope unchanged since a completed comment pass this session.

**Delegate** via `Task` with `subagent_type: code-commenter`:

```markdown
Scope: translation units in SCOPE_FILES (committed + uncommitted) under src/, include/, and tests/ (.c under tests/unit/, tests/harness/, tests/soak/).
Follow .cursor/skills/code-commenter/SKILL.md and .cursor/agents/code-commenter.md.
Comment-only edits; do not change executable behavior.
Report using the code-commenter skill output format.
```

## Step 3: docs-steward

**Skip when:** user opted out; diff unchanged since a completed documentation pass this session; comment-only `src/` with no doc/workflow impact (confirm in summary).

**Do not skip** on tooling-only branches that change `.cursor/` workflow files, agent guidance, or other documentation signals - those still need inventory and alignment before draft PR.

**Delegate** via `Task` with `subagent_type: docs-steward`:

```markdown
Scope: full SCOPE_FILES (committed + uncommitted vs base) for change signals; edit documentation paths only.
Follow .cursor/skills/documentation-maintainer/SKILL.md and .cursor/agents/docs-steward.md.
Report using the documentation-maintainer skill output format.
```

## Between steps

- If a subagent committed or edited files, parent recomputes `SCOPE_FILES` before the next step.
- Do not open draft PR until all non-skipped steps complete.
- Draft PR, GitHub board moves stay in [agent-workflow](../../rules/agent-workflow.mdc). Root `DEV_PLAN.md` per AGENTS **DEV_PLAN updates** (inline Contains Done after merge; no v1 Done sections).

## Deduplication guard

Before each subagent, check conversation history for a completed pass report on the **same base ref and `SCOPE_FILES` set** (committed + uncommitted).

| If found | Action |
|----------|--------|
| Complete report for this pass on same scope | Skip step; note "already completed this session" in combined output |
| New commits or working-tree edits changed `SCOPE_FILES` | Re-run affected steps |
| User explicitly requests re-audit | Re-run requested steps |
| Prior report noted failures or incomplete work | Re-run that step |

## Combined output format

```markdown
## Pre-draft PR passes

### Prerequisites
- make test*: pass | fail (stopped)

### Steps
| Step | Agent | Result | Notes |
|------|-------|--------|-------|
| 1 | test-auditor | run / skipped | ... |
| 2 | code-commenter | run / skipped | ... |
| 3 | docs-steward | run / skipped | ... |

### Summary
- ready for draft PR: yes | no - reason
- tests added/updated: ...
- comments added: ...
- docs updated: ...

### Risks
- only if a pass may need re-run soon (e.g. active migration)
```

## Escape hatches

- **Single pass only:** user names one pass; delegate that subagent only via the child skill/agent pair.
- **Inline pass:** parent follows child skill without `Task` when a full subagent run is unnecessary (simple docs-only PR).
- **User skip:** honor explicit opt-out; note in combined output.

## Out of scope unless asked

- Opening draft PR, linking issues, project board status
- `pr-after-push` (after git push to open PR)
- Playtesting ([play-tester](../play-tester/SKILL.md))
- Replacing per-pass gate rules or child skill bodies
