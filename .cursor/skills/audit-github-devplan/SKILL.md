---
name: audit-github-devplan
description: >-
  Audits alignment between GitHub issues, milestones, project board, blocked-by
  dependencies, and DEV_PLAN.md. Use when the user asks to audit the roadmap,
  review milestone categorization, check DEV_PLAN vs GitHub, reconcile execution
  order, or verify project board Priority vs stack order.
disable-model-invocation: true
---

# Audit GitHub issues and DEV_PLAN

Cross-check **GitHub** (issues, milestones, project #1, blocked-by) against **[`DEV_PLAN.md`](../../../DEV_PLAN.md)**. Report mismatches; apply fixes only when the user asks.

Canonical policy: [`AGENTS.md`](../../../AGENTS.md) **DEV_PLAN updates** and DEV_PLAN **Execution order** section.

## When to run

- User asks to audit, reconcile, or review roadmap / milestones / DEV_PLAN
- After bulk milestone renames, issue moves, or dependency wiring
- When board order looks wrong vs Priority (P0/P1/P2)

## Concepts (do not conflate)

| Mechanism | What it is | Sort / meaning |
|-----------|------------|----------------|
| **GitHub Milestone** | Theme label on an issue | Not strict schedule |
| **DEV_PLAN execution order** | Suggested pull order + mermaid in DEV_PLAN | Dependency / workflow sequence |
| **blocked-by** | Native issue Relationships | Source of truth for blockers |
| **Project Priority** | P0 / P1 / P2 custom field | Coarse urgency tier; **does not** auto-sort columns |
| **Project stack order** | Global item position (`updateProjectV2ItemPosition`) | Visual order within a Status column |

Priority P0→P1→P2 and execution-order stack **will diverge** (e.g. #47 P1 before #104 P0). That is expected unless the user asks to sort by Priority instead.

## DEV_PLAN edit rules (audit against these)

| Rule | Check |
|------|-------|
| Do **not** add new issue sections for newly created BAU issues | Grep new `#N` in DEV_PLAN vs issues created recently |
| Edit DEV_PLAN on draft PR only per AGENTS table | Issue already has section, or milestone already in DEV_PLAN |
| Milestones are **themes**, not "Phase N" wording | No `Phase 1`–style milestone labels in docs |
| Harness layers in docs | `unit` / `soak` / snapshot wording; not Phase B/C unless explicitly historical |

Issues **may** appear in DEV_PLAN without a milestone if they were listed before the milestone-only cleanup; do not treat "no milestone" alone as delete-worthy.

## Audit workflow

Copy and track:

```text
Audit progress:
- [ ] 1. Inventory DEV_PLAN issue refs
- [ ] 2. Fetch GitHub milestone + state per ref
- [ ] 3. Compare DEV_PLAN section vs milestone title
- [ ] 4. Open roadmap: project board + Priority
- [ ] 5. blocked-by vs execution order
- [ ] 6. Terminology / doc drift
- [ ] 7. Write report (fix only if asked)
```

### 1. Inventory DEV_PLAN

```bash
grep -oE '#[0-9]+' DEV_PLAN.md | sort -u
```

Note section headings (milestone blocks) each issue sits under.

### 2. GitHub issue snapshot

For listed numbers (batch):

```bash
gh issue list --state all --limit 500 --json number,title,state,milestone \
  | python3 -c "
import json, sys
nums = {71, 47}  # from grep
for i in json.load(sys.stdin):
    if i['number'] in nums:
        m = (i.get('milestone') or {}).get('title')
        print(i['number'], i['state'], m)
"
```

Per-issue dependencies:

```bash
gh api repos/ianmays/dosmud/issues/48 --jq '.issue_dependencies_summary'
```

### 3. Milestone vs DEV_PLAN section

Flag:

- Issue in DEV_PLAN under milestone **A**, GitHub milestone **B**
- Open milestone issue with no project item
- Closed issue still **open** wording in DEV_PLAN (missing Done ✅)

Suggest moves: `gh issue edit <N> --milestone "<title>"` (match [DEV_PLAN milestone index](https://github.com/ianmays/dosmud/milestones)).

### 4. Project board (dosmud #1)

```bash
gh project item-list 1 --owner ianmays --format json --limit 200
gh project field-list 1 --owner ianmays --format json
```

For open roadmap issues, record **Status**, **Priority**, and column stack order (GraphQL item list order filtered by Status).

GraphQL (status + priority on items):

```bash
gh api graphql -f query='
query {
  user(login: "ianmays") {
    projectV2(number: 1) {
      items(first: 100) {
        nodes {
          id
          content { ... on Issue { number state } }
          fieldValues(first: 20) {
            nodes {
              ... on ProjectV2ItemFieldSingleSelectValue {
                name
                field { ... on ProjectV2SingleSelectField { name } }
              }
            }
          }
        }
      }
    }
  }
}'
```

### 5. blocked-by vs execution order

Read DEV_PLAN **Execution order** and dependency prose. For each edge documented there:

```bash
BLOCKER_ID=$(gh api repos/ianmays/dosmud/issues/71 --jq .id)
gh api repos/ianmays/dosmud/issues/47/dependencies/blocked_by \
  --method POST --input - <<< "{\"issue_id\": $BLOCKER_ID}"
```

`gh issue edit` has no blocked-by flags; use REST `dependencies/blocked_by`. Duplicate add returns validation error (safe to ignore).

Verify: blocked issue should appear **after** blockers in execution order list.

### 6. Terminology drift

```bash
grep -n 'Phase [0-9]' DEV_PLAN.md AGENTS.md docs/*.md README.md
grep -n 'Phase [ABC]' docs/testing.md
```

## Report format

Deliver to the user:

```markdown
## Roadmap audit

### Summary
- N DEV_PLAN refs; M open on board; K mismatches

### Milestone mismatches
| Issue | DEV_PLAN section | GitHub milestone | Action |

### DEV_PLAN hygiene
- (issues to mark Done ✅, stale sections, BAU refs to remove)

### Dependencies
- (missing / extra blocked-by vs DEV_PLAN)

### Project board
- Backlog / Parked / Agent-ready stack vs execution order
- Note if Priority order differs (expected)

### Recommended actions
- (numbered; do not execute unless user confirms)
```

## Applying fixes (only when asked)

Typical batch:

1. `gh issue edit` milestone moves
2. REST blocked-by edges
3. `gh project item-edit` for Priority
4. GraphQL `updateProjectV2ItemPosition` for stack order within Status (sort by execution rank from DEV_PLAN, not P0/P1/P2)
5. Docs PR for DEV_PLAN / AGENTS / testing.md terminology

After doc changes: branch + draft PR per [`agent-workflow.mdc`](../../rules/agent-workflow.mdc). GitHub-only metadata may ship without a PR if user prefers.

## Reorder script pattern

Within each **Status**, sort open roadmap issues by DEV_PLAN execution rank; non-roadmap items keep relative order after them. See [examples.md](examples.md).

## Checklist

- [ ] Report distinguishes Priority vs stack order
- [ ] DEV_PLAN "do not add new issues" respected in recommendations
- [ ] blocked-by checked against DEV_PLAN dependency prose
- [ ] No drive-by DEV_PLAN edits outside user scope
- [ ] Milestone titles match DEV_PLAN headings (no Phase prefix)

## Additional resources

- Sample audit output: [examples.md](examples.md)
