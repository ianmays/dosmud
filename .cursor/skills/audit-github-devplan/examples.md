# Audit examples (dosmud)

## Priority vs stack order (expected mismatch)

Execution order (Parked subset):

```text
#71 P0 → #47 P1 → #104 P0 → #100 P1 → … → #52 P2 → #55 P1 → #7 P2
```

#55 is P1 but sits after P2 issues because Content Expansion (#55) follows Advanced Mechanics in the dependency graph. Explain this in the audit report; do not flag as a bug unless the user wanted Priority-sorted columns.

## Milestone mismatch example

| Issue | DEV_PLAN section | GitHub milestone | Action |
|-------|------------------|------------------|--------|
| #71 | Engine Enhancements | Advanced Architecture | `gh issue edit 71 --milestone "Advanced Architecture"`; move DEV_PLAN section under milestone 5 heading |
| #132 | Content Expansion | Advanced Mechanics | move to milestone 8; update DEV_PLAN cross-ref |

## blocked-by REST

```bash
# #48 blocked by #71 and #47
for blocker in 71 47; do
  BID=$(gh api repos/ianmays/dosmud/issues/$blocker --jq .id)
  gh api repos/ianmays/dosmud/issues/48/dependencies/blocked_by \
    --method POST --input - <<< "{\"issue_id\": $BID}" 2>&1 || true
done
gh api repos/ianmays/dosmud/issues/48 --jq '.issue_dependencies_summary'
# expect total_blocked_by: 2
```

## GraphQL reorder (execution order, one Status)

```python
# Pseudocode: within "Parked", sort roadmap open issues by EXEC_ORDER rank
EXEC_ORDER = [74, 82, 34, 72, 71, 47, 16, 104, ...]  # from DEV_PLAN
rank = {n: i for i, n in enumerate(EXEC_ORDER)}

# For each status group: roadmap.sort(key=lambda x: rank[x.num])
# Rebuild global item id list; then:
prev = None
for item_id in new_global_order:
    # mutation updateProjectV2ItemPosition(projectId, itemId, afterId=prev)
    prev = item_id
```

Project id: `PVT_kwHOAAzqPM4BW5KP` (verify with `gh project view 1 --owner ianmays --format json`).

## Report snippet (milestone audit)

```markdown
## Roadmap audit

### Summary
- 28 DEV_PLAN refs checked; 33 open milestone issues on board; 2 milestone mismatches

### Milestone mismatches
| Issue | DEV_PLAN section | GitHub milestone | Action |
|-------|------------------|------------------|--------|
| #71 | Engine Enhancements | Advanced Architecture | DEV_PLAN section already moved; GitHub OK |

### DEV_PLAN hygiene
- #87, #105 correctly absent (BAU, no milestone sections)
- Phase B/C still in docs/testing.md → rename to unit coverage / soak (#95 / #116)

### Project board
- Backlog: #82, #34, #72 matches execution order
- Parked: execution order OK; Priority not monotonic (by design)

### Recommended actions
1. docs PR: drop Phase B/C in testing.md
2. no milestone moves needed
```
