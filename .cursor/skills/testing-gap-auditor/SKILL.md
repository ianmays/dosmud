---
name: testing-gap-auditor
description: >-
  Audit branch diffs for missing unit or snapshot tests before draft PR.
  Run scripts/check-test-gaps.sh, interpret coverage, and add tests or
  tests/.test-gap-waiver. Use after make test* on gameplay branches or when
  CI test-gap step fails.
---

# Testing gap auditor

**Procedure (this skill).** Diff vs `main`, obligation matrix, script, coverage, report.

**Policy (canonical).** [`AGENTS.md`](../../../AGENTS.md) **Testing pass** and [When to add or update tests](../../../docs/testing.md#when-to-add-or-update-tests).

**Judgement (agent).** [`.cursor/agents/test-auditor.md`](../../agents/test-auditor.md) when delegating a full pass (may author tests).

**Gate (rule).** [testing-gap-after-implement.mdc](../../rules/testing-gap-after-implement.mdc).

**CI.** PRs to `main` run `scripts/check-test-gaps.sh` as a **hard fail** (no `continue-on-error`). Escape hatch: commit [`tests/.test-gap-waiver`](../../../tests/.test-gap-waiver) with a one-line reason.

## When to run

1. After `make test`, `make test-run`, and `make test-unit` (or `make test-all`) on a behavioral feature branch.
2. Before code-commenter and documentation passes.
3. When CI **Check test gaps** fails on a PR.
4. User asks whether unit/snapshot coverage is missing.

## Skip when

- Branch diff vs `main` has no `src/`, `include/`, `tests/unit/`, `tests/regression/`, or `Makefile` `SNAPSHOT_TESTS` changes (script exits 0).
- `tests/.test-gap-waiver` is committed with reason.
- User opts out.
- Diff unchanged since a completed test-gap pass this session.

## Checklist

```text
Test-gap pass:
- [ ] 1. Diff inventory vs main (src/, include/, tests/, Makefile)
- [ ] 2. Classify change (obligation matrix below)
- [ ] 3. sh scripts/check-test-gaps.sh origin/main
- [ ] 4. make test-unit-coverage if in-scope src/ changed
- [ ] 5. Direct unit tests for new exports (#90 lesson)
- [ ] 6. Snapshot obligation for player-visible paths
- [ ] 7. Report; fix tests or waiver; PR Test plan / issue Testing subsection
```

## Obligation matrix

| Diff signal | Unit (`tests/unit/unit_*.c`) | Snapshots (`tests/regression/`, `SNAPSHOT_TESTS`) |
|-------------|------------------------------|---------------------------------------------------|
| New/changed exported API in scope `.h` | **Required** direct test | If output changes |
| New verb / changed player-visible output | Yes | **Required** |
| Slice `*_cmd_*` moved/added | **Required** in slice `unit_*.c` | If output changes |
| `game.c` router/tick only | Only if semantics change | No unless output changes |
| `grendr` / `txtres` copy-only | No | **Required** |
| `grendr` / `fmt` render logic | When logic changes | If output changes |
| Refactor, behavior preserved | Update if APIs moved | Only if `.expect` drift |

**In-scope modules (coverage bar):** `command`, `invent`, `combat`, `game`, `genc`, `wanderer`, `dialogue`, `gatmos`, `world`, `gprog`, `items`, `fmt`, `gout`, `testharn` (see `COVERAGE_MODULES` in `Makefile`).

## Unit file map

| `src/` module | Unit suite |
|---------------|------------|
| `command.c` | `unit_cmd.c` |
| `invent.c` | `unit_inv.c` |
| `combat.c` | `unit_cbt.c` |
| `dialogue.c` | `unit_dial.c` |
| `world.c` | `unit_wrld.c` |
| `game.c` | `unit_game.c` |
| `gout.c` | `unit_gout.c` |
| `genc.c` | `unit_genc.c` |
| `gprog.c` | `unit_gprog.c` |
| `gatmos.c` | `unit_gatmos.c` |
| `wanderer.c` | `unit_wandr.c` |
| `fmt.c` | `unit_fmt.c` |
| `items.c` | `unit_item.c` |
| `testharn.c` | `unit_tharn.c`, `unit_harn.c` |

Abbreviated basenames match [`docs/testing.md`](../../../docs/testing.md#when-to-add-or-update-tests).

## Script

```sh
git fetch origin main
sh scripts/check-test-gaps.sh origin/main
```

- Exit **0:** pass or waived.
- Exit **1:** hard gap; add tests or `tests/.test-gap-waiver`.
- `TEST_GAP_WAIVE=1`: local debug only; do not set in CI.

## Waiver file

Path: `tests/.test-gap-waiver`

Example (one line):

```text
behavior-preserving grendr refactor; snapshots unchanged per make test-run
```

Commit in the PR when heuristics misfire under the hard CI gate. Do not leave a permanent waiver on `main` without cause.

## Output format

```markdown
## Test-gap audit

### Summary
- script: pass | fail (N gaps)
- coverage: pass | below 90% branch: ...

### Unit gaps
| Module | Signal | Required action |

### Snapshot gaps
| Path | Signal | Required action |

### Waivers
- (none) or reason + path

### Recommended next step
- add tests / update snapshots / none - reason
```

## What the agent may do

- Add or update `tests/unit/unit_*.c`, `tests/regression/*`, `Makefile` `SNAPSHOT_TESTS`.
- Re-run `make test`, `make test-run`, `make test-unit`.
- Commit `tests/.test-gap-waiver` when intentional (document reason in report).

Do **not** change gameplay behavior except through tests. Use `unit_game_fresh`, `plat_seed_rng`, `game_roll_inject_*`, and `@fixture` per [`docs/testing.md`](../../../docs/testing.md).
