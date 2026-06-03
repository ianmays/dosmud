---
name: testing-gap-auditor
description: >-
  Audit branch diffs for missing unit or snapshot tests before draft PR.
  Run scripts/check-test-gaps.sh, interpret coverage, and add tests or
  Use after make test* on gameplay branches or when
  CI test-gap step fails.
---

# Testing gap auditor

**Procedure (this skill).** Diff vs `main`, obligation matrix, script, coverage, report.

**Policy (canonical).** [`AGENTS.md`](../../../AGENTS.md) **Testing pass** and [When to add or update tests](../../../docs/testing.md#when-to-add-or-update-tests).

**Judgement (agent).** [`.cursor/agents/test-auditor.md`](../../agents/test-auditor.md) when delegating a full pass (may author tests).

**Gate (rule).** [testing-gap-after-implement.mdc](../../rules/testing-gap-after-implement.mdc).

**CI.** PRs run `scripts/check-test-gaps.sh` in **informative** mode (`TEST_GAP_INFORMATIVE=1`): gaps are logged, job stays green. Fix gaps or tune the script before merge; no CI waiver.

## When to run

1. After `make test`, `make test-run`, and `make test-unit` (or `make test-all`) on a behavioral feature branch.
2. Before code-commenter and documentation passes.
3. When CI **Check test gaps** fails on a PR.
4. User asks whether unit/snapshot coverage is missing.

## Skip when

- Branch diff vs `main` has no `src/`, `include/`, `tests/unit/`, `tests/regression/`, or `Makefile` `SNAPSHOT_TESTS` changes (script exits 0).
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
- [ ] 7. Report; fix tests; PR Test plan / issue Testing subsection
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

**In-scope modules:** `COVERAGE_MODULES` in the [`Makefile`](../../../Makefile). **Unit suites:** `tests/unit/unit_*.c` that `#include "<module>.h"` (resolved by `scripts/check-test-gaps.sh`). Optional overrides: [`tests/unit/module-map`](../../../tests/unit/module-map). Naming abbreviations (e.g. `command` → `unit_cmd.c`): [`docs/testing.md`](../../../docs/testing.md#when-to-add-or-update-tests) and `UNIT_TEST_SRC` in the Makefile - do not duplicate a module table here.

## Script

```sh
git fetch origin main
sh scripts/check-test-gaps.sh origin/main
```

- Exit **0:** pass or waived.
- Exit **1:** gaps found (default locally; required before draft PR).
- Exit **0** with gap messages: `TEST_GAP_INFORMATIVE=1` (CI only).
- `TEST_GAP_WAIVE=1`: skip script locally; not used in CI.

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

### Recommended next step
- add tests / update snapshots / none - reason
```

## What the agent may do

- Add or update `tests/unit/unit_*.c`, `tests/regression/*`, `Makefile` `SNAPSHOT_TESTS`.
- Re-run `make test`, `make test-run`, `make test-unit`.
Do **not** change gameplay behavior except through tests. Use `unit_game_fresh`, `plat_seed_rng`, `game_roll_inject_*`, and `@fixture` per [`docs/testing.md`](../../../docs/testing.md).
