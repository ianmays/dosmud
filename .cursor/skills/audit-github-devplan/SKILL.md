---
name: audit-github-devplan
description: >-
  Audits alignment between GitHub issues, milestones, project board, blocked-by
  dependencies, and DEV_PLAN.md. Use when the user asks to audit the roadmap,
  review milestone categorization, check DEV_PLAN vs GitHub, reconcile execution
  order, verify project board Priority vs stack order, or compare Size (XS–XL)
  on the board to DEV_PLAN.
disable-model-invocation: true
---

# Audit GitHub issues and DEV_PLAN

**See also:** proactive setup when creating milestone issues - [milestone-issue-hygiene](../milestone-issue-hygiene/SKILL.md); documentation pass before draft PR - [documentation-maintainer](../documentation-maintainer/SKILL.md). This skill is for **reactive** audits and reconciliation.

Cross-check **GitHub** (issues, milestones, project #1, blocked-by) against **[`DEV_PLAN.md`](../../../DEV_PLAN.md)** (Roadmap v2) and [`docs/archive/DEV_PLAN_v1_engine_foundation.md`](../../../docs/archive/DEV_PLAN_v1_engine_foundation.md) when v1 history matters. Report mismatches; apply fixes only when the user asks.

## Roadmap v2 guard

Root `DEV_PLAN.md` is a **strategic index** (lanes, spine, parked systems) - not the v1 milestone ledger. **Do not** recommend adding v1 milestone tables, execution-order mermaid, per-issue stubs, or **Done ✅** sections to root `DEV_PLAN.md` unless the user explicitly requests it. v1 Done-marker and milestone-table audits apply to the **archive** only.

Canonical policy: [`AGENTS.md`](../../../AGENTS.md) **DEV_PLAN updates** and Roadmap v2 lanes/spine in root `DEV_PLAN.md`.

## When to run

- User asks to audit, reconcile, or review roadmap / milestones / DEV_PLAN
- After bulk milestone renames, issue moves, or dependency wiring
- When board order looks wrong vs Priority (P0/P1/P2)

## Concepts (do not conflate)

| Mechanism | What it is | Sort / meaning |
|-----------|------------|----------------|
| **GitHub Milestone** | Theme label on an issue | Not strict schedule |
| **DEV_PLAN (Roadmap v2)** | Lanes, spine, parked systems in root `DEV_PLAN.md` | Strategic sequencing; not Agent-ready queue |
| **DEV_PLAN v1 archive** | [`docs/archive/DEV_PLAN_v1_engine_foundation.md`](../../../docs/archive/DEV_PLAN_v1_engine_foundation.md) | Historical milestone tables, Done markers, execution mermaid |
| **blocked-by** | Native issue Relationships | Source of truth for blockers |
| **Project Priority** | P0 / P1 / P2 custom field | Coarse urgency tier; **does not** auto-sort columns |
| **Project Size** | XS–XL custom field on project #1 | Relative effort / blast radius; calibrate against archive **Relative size** legend |
| **Project stack order** | Global item position (`updateProjectV2ItemPosition`) | Visual order within a Status column |

Priority P0→P1→P2 and execution-order stack **will diverge** (e.g. #47 P1 before #104 P0). That is expected unless the user asks to sort by Priority instead. **Size** is independent of both Priority and stack order.

**Board Status (Planning, Backlog, Parked, etc.):** which column an issue sits in changes often. Document layout exceptions in root `DEV_PLAN.md` lanes/spine or the v1 archive when maintaining history. Do **not** duplicate per-issue column placement in skills, rules, or audit reports. When checking stack order, compare board order within each Status group against Roadmap v2 spine/lanes or archive **Execution order** when auditing v1 history; do not recommend Status moves unless the user asks.

## DEV_PLAN edit rules (audit against these)

| Rule | Check |
|------|-------|
| Do **not** add new issue sections for newly created BAU issues to root `DEV_PLAN.md` | Grep new `#N` in root DEV_PLAN vs issues created recently |
| Root `DEV_PLAN.md` edits | Lane/spine updates only when user requests; no v1 tables/mermaid/Done by default |
| **Done ✅** on v1 archive section | Only when explicitly maintaining [`docs/archive/DEV_PLAN_v1_engine_foundation.md`](../../../docs/archive/DEV_PLAN_v1_engine_foundation.md) |
| Initial v1 DEV_PLAN row + stub | Archive only when user requests v1 history maintenance - not default for new issues |
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
- [ ] 4. Open roadmap: project board + Priority + Size
- [ ] 5. Compare board Size vs archive Size column (open issues; v1 history)
- [ ] 6. blocked-by vs archive execution order (or spine/lanes for v2)
- [ ] 7. Terminology / doc drift
- [ ] 8. Write report (fix only if asked)
```

### 1. Inventory DEV_PLAN

Grep **root** Roadmap v2 and **archive** when v1 tables or Done markers matter:

```bash
grep -h -oE '#[0-9]+' DEV_PLAN.md docs/archive/DEV_PLAN_v1_engine_foundation.md | sort -u
```

Note lane/spine placement in root `DEV_PLAN.md`; note milestone blocks in the archive when auditing v1 history.

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

- Issue in archive under milestone **A**, GitHub milestone **B**
- Open milestone issue with no project item
- Closed issue still **open** wording in v1 archive (missing Done ✅)

Suggest moves: `gh issue edit <N> --milestone "<title>"` (match [GitHub milestones](https://github.com/ianmays/dosmud/milestones)). For v2, compare issue numbers to root lanes/spine and project #1 Agent-ready stack.

### 4. Project board (dosmud #1)

```bash
gh project item-list 1 --owner ianmays --format json --limit 200
gh project field-list 1 --owner ianmays --format json
```

For open roadmap issues, record **Priority**, **Size**, and stack order within each Status column (GraphQL item list order filtered by Status). Root Roadmap v2 is canonical for lanes/spine; use the v1 archive for historical Status vs pull-order notes.

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

### 5. Size vs DEV_PLAN (v1 archive)

**v1 history only.** Read [`docs/archive/DEV_PLAN_v1_engine_foundation.md`](../../../docs/archive/DEV_PLAN_v1_engine_foundation.md) **Relative size** legend and milestone **Size** columns. For Roadmap v2, compare board **Size** to project calibration (XS–XL) without expecting root `DEV_PLAN.md` tables.

For each open roadmap issue on the board (when auditing v1 tables):

```bash
gh project item-list 1 --owner ianmays --format json --limit 200 \
  | python3 -c "
import json, sys
EXEC = {71, 47, ...}  # open roadmap set from archive tables
for it in json.load(sys.stdin)['items']:
    n = it.get('content', {}).get('number')
    if n in EXEC:
        print(n, it.get('size','-'))
"
```

Flag when board **Size** differs from archive table for the same issue.

### 6. blocked-by vs execution order (v1 archive)

**v1 history only.** Read archive **Execution order** and dependency prose. For Roadmap v2, compare blocked-by edges to spine/lane intent and project #1 stack order instead of v1 mermaid.

For each edge documented in the archive:

```bash
BLOCKER_ID=$(gh api repos/ianmays/dosmud/issues/71 --jq .id)
gh api repos/ianmays/dosmud/issues/47/dependencies/blocked_by \
  --method POST --input - <<< "{\"issue_id\": $BLOCKER_ID}"
```

`gh issue edit` has no blocked-by flags; use REST `dependencies/blocked_by`. Duplicate add returns validation error (safe to ignore).

Verify: blocked issue should appear **after** blockers in execution order list.

### 7. Terminology drift

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
- (v1 archive: issues to mark Done ✅, stale sections; root v2: lane/spine drift only)

### Dependencies
- (missing / extra blocked-by vs archive execution prose or v2 spine)

### Size mismatches
| Issue | Archive Size (v1) | Board Size | Action |

### Project board
- Stack order vs archive execution order or v2 spine (per Status column, if user asked)
- Priority vs stack order (expected divergence)
- Status column placement: see DEV_PLAN only; do not flag unless user asks

### Recommended actions
- (numbered; do not execute unless user confirms)
```

## Applying fixes (only when asked)

Typical batch:

1. `gh issue edit` milestone moves
2. REST blocked-by edges
3. `gh project item-edit` for Priority
4. GraphQL `updateProjectV2ItemPosition` for stack order within Status (sort by archive execution rank or v2 spine/lanes, not P0/P1/P2)
5. Docs PR for root `DEV_PLAN.md` lanes/spine, archive, AGENTS / testing.md terminology

After doc changes: branch + draft PR per [`agent-workflow.mdc`](../../rules/agent-workflow.mdc). GitHub-only metadata may ship without a PR if user prefers.

## Reorder script pattern

Within each **Status**, sort open roadmap issues by archive execution rank or Roadmap v2 spine; non-roadmap items keep relative order after them. See [examples.md](examples.md).

## Checklist

- [ ] Report distinguishes Priority vs stack order vs Size
- [ ] Size checked against archive tables (v1) or board calibration (v2)
- [ ] DEV_PLAN "do not add new issues" respected in recommendations
- [ ] blocked-by checked against archive dependency prose or v2 spine
- [ ] No drive-by DEV_PLAN edits outside user scope
- [ ] Milestone titles match DEV_PLAN headings (no Phase prefix)

## Additional resources

- Sample audit output: [examples.md](examples.md)
