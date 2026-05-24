# Audit examples (dosmud)

## Priority vs stack order (expected mismatch)

Execution order (subset) vs Priority labels:

```text
#71 P0 → #47 P1 → #104 P0 → #100 P1 → … → #52 P2 → #49 P2 → #55 P1 → #7 P2
```

#55 is P1 but follows P2 issues because Content Expansion (#55) comes after Advanced Mechanics in the dependency graph. Explain in the audit report; do not flag unless the user wanted Priority-sorted columns.

## Size audit

DEV_PLAN **Relative size** legend and milestone tables are canonical. Example check:

| Issue | DEV_PLAN | Board | OK? |
|-------|----------|-------|-----|
| #71 | XL | XL | yes |
| #128 | S | S | yes |
| #47 | L | M | **drift** - update board or DEV_PLAN |

```bash
gh project item-list 1 --owner ianmays --format json --limit 200 \
  | python3 -c "import json,sys; [print(i['content']['number'], i.get('size','-')) for i in json.load(sys.stdin)['items'] if i.get('content',{}).get('number')==71]"
```

## Milestone mismatch example

| Issue | DEV_PLAN section | GitHub milestone | Action |
|-------|------------------|------------------|--------|
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
EXEC_ORDER = [74, 82, 34, 72, 71, 47, 16, 104, ...]  # from DEV_PLAN
rank = {n: i for i, n in enumerate(EXEC_ORDER)}
# within each Status: sort roadmap items by rank; updateProjectV2ItemPosition (afterId)
```

Project id: `PVT_kwHOAAzqPM4BW5KP` (verify with `gh project view 1 --owner ianmays --format json`).

## Report snippet (clean audit)

```markdown
## Roadmap audit

### Summary
- 48 DEV_PLAN refs; 33 open roadmap on board; 0 mismatches

### Size mismatches
- (none)

### Project board
- Stack order matches DEV_PLAN execution order (column checked per user request)
- Priority not monotonic vs stack (expected)

### Recommended actions
- none
```
