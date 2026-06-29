# Audit examples (dosmud)

## Priority vs stack order (expected mismatch)

Execution order (subset) vs Priority labels:

```text
#71 P0 → #47 P1 → #104 P0 → #100 P1 → … → #52 P2 → #49 P2 → #55 P1 → #7 P2
```

#55 is P1 but follows P2 issues because Content Expansion (#55) comes after Advanced Mechanics in the dependency graph. Explain in the audit report; do not flag unless the user wanted Priority-sorted columns.

## Size audit (v1 archive)

For v1 history audits, read [`docs/archive/DEV_PLAN_v1_engine_foundation.md`](../../../docs/archive/DEV_PLAN_v1_engine_foundation.md) **Relative size** legend and milestone tables. For Roadmap v2, compare board **Size** (XS–XL) to issue scope without expecting root `DEV_PLAN.md` tables.

Example check (archive table vs board):

| Issue | Archive Size | Board | OK? |
|-------|--------------|-------|-----|
| #71 | XL | XL | yes |
| #128 | S | S | yes |
| #47 | L | M | **drift** - update board or archive |

```bash
gh project item-list 1 --owner ianmays --format json --limit 200 \
  | python3 -c "import json,sys; [print(i['content']['number'], i.get('size','-')) for i in json.load(sys.stdin)['items'] if i.get('content',{}).get('number')==71]"
```

## Milestone mismatch example (v1 archive)

| Issue | Archive section | GitHub milestone | Action |
|-------|-----------------|------------------|--------|
| #71 | Advanced Architecture | Advanced Architecture | none |
| #132 | Advanced Mechanics | Advanced Mechanics | none |

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

Use only when the user asks to fix stack order within a column:

```python
EXEC_ORDER = [74, 82, 34, 72, 71, 47, 16, 104, ...]  # from v1 archive execution order
rank = {n: i for i, n in enumerate(EXEC_ORDER)}
# within each Status: sort roadmap items by rank; updateProjectV2ItemPosition (afterId)
```

Project id: `PVT_kwHOAAzqPM4BW5KP` (verify with `gh project view 1 --owner ianmays --format json`).

## Report snippet (clean audit)

```markdown
## Roadmap audit

### Summary
- 48 archive refs; 33 open roadmap on board; 0 mismatches

### Size mismatches
- (none)

### Project board
- Stack order matches archive execution order or Roadmap v2 spine (column checked per user request)
- Priority not monotonic vs stack (expected)

### Recommended actions
- none
```
