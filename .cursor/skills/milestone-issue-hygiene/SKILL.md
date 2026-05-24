---
name: milestone-issue-hygiene
description: >-
  After creating or on request for an existing issue: set GitHub project Size
  and Priority, wire blocked-by relationships, update DEV_PLAN when the milestone
  is tracked there, and align project stack order. Use when filing milestone
  issues or when the user asks for roadmap hygiene on #N.
---

# Milestone issue hygiene

Run **in the same turn** as `gh issue create` when the new issue has a GitHub **Milestone** listed in [`DEV_PLAN.md`](../../../DEV_PLAN.md) milestone index (line 38). Also run retroactively when the user asks for **roadmap hygiene for #N**.

Reactive audits and bulk fixes: [`.cursor/skills/audit-github-devplan/SKILL.md`](../audit-github-devplan/SKILL.md).

## When to run

- Agent or human created an issue with a DEV_PLAN-tracked milestone
- User: "roadmap hygiene for #145", "size and prioritize #N", "add #N to DEV_PLAN"
- **Not** for BAU issues without a milestone, or milestones not in DEV_PLAN (board + issue only)

## Checklist

Copy and track:

```text
Milestone hygiene for #N:
- [ ] 1. Labels + project item
- [ ] 2. Size (XS–XL) matches issue scope in body/title
- [ ] 3. Priority (P0–P2)
- [ ] 4. blocked-by edges (REST) + DEV_PLAN dependency prose
- [ ] 5. Issue body: scope, Out of scope, Testing subsection
- [ ] 6. DEV_PLAN (if milestone section exists)
- [ ] 7. Project stack order within Status column
- [ ] 8. Status column (Backlog default; blocked → stay Backlog)
- [ ] 9. Hygiene comment on issue (summary + skill link)
```

### 1. Labels and project

```sh
gh issue edit <N> --add-label gameplay   # or tooling, documentation, etc.
# agent-created issues: --add-label agent
gh project item-add 1 --owner ianmays --url https://github.com/ianmays/dosmud/issues/<N>
```

Skip `item-add` if the issue is already on [project #1](https://github.com/users/ianmays/projects/1).

### 2. Size (project field)

Align with DEV_PLAN **Relative size** legend:

| Size | Meaning |
|------|---------|
| XS | single trivial change |
| S | narrow feature or tooling slice |
| M | one subsystem feature or refactor |
| L | major mechanism or platform path |
| XL | foundational or multi-area epic |

**Title and body must match Size.** If scope is M, defer epics to Out of scope or follow-up issues.

Fetch option ids (or use stable ids below):

```sh
gh project field-list 1 --owner ianmays --format json
```

dosmud project #1 Size field `PVTSSF_lAHOAAzqPM4BW5KPzhSJ4X8`: XS `6c6483d2`, S `f784b110`, M `7515a9f1`, L `817d0097`, XL `db339eb2`.

```sh
gh project item-edit --id <ITEM_ID> --project-id PVT_kwHOAAzqPM4BW5KP \
  --field-id PVTSSF_lAHOAAzqPM4BW5KPzhSJ4X8 \
  --single-select-option-id <SIZE_OPTION_ID>
```

Get `<ITEM_ID>` from `gh project item-list 1 --owner ianmays --format json` (filter by issue number). **Always** resolve by issue number before `item-edit`; a duplicate `item-add` creates a second card and edits may land on the wrong id.

### 3. Priority (project field)

Heuristics:

| Priority | Use when |
|----------|----------|
| P0 | architectural foundation gates (#71-style), blocking many downstream issues |
| P1 | high-value mechanics or tooling on the critical path |
| P2 | deferred polish, follow-ups, most Advanced Mechanics backlog |

Priority field `PVTSSF_lAHOAAzqPM4BW5KPzhSJ4X4`: P0 `79628723`, P1 `0a877460`, P2 `da944a9c`.

```sh
gh project item-edit --id <ITEM_ID> --project-id PVT_kwHOAAzqPM4BW5KP \
  --field-id PVTSSF_lAHOAAzqPM4BW5KPzhSJ4X4 \
  --single-select-option-id <PRIORITY_OPTION_ID>
```

Priority does **not** have to match stack order (see audit skill).

### 4. blocked-by (issue Relationships)

`gh issue edit` has no blocked-by flags. Use REST:

```sh
BLOCKER_ID=$(gh api repos/ianmays/dosmud/issues/<blocker> --jq .id)
gh api repos/ianmays/dosmud/issues/<blocked>/dependencies/blocked_by \
  --method POST --input - <<< "{\"issue_id\": $BLOCKER_ID}"
```

- Do **not** block on PRs (issues only)
- Mirror new chains in DEV_PLAN **Dependencies** prose (~line 77)
- Verify: `gh api repos/ianmays/dosmud/issues/<N> --jq '.issue_dependencies_summary'`

### 5. Issue body

Required for milestone-tracked roadmap issues:

- **Scope** matching Size (M = one deliverable slice)
- **Out of scope** for deferred work (avoid title/body drift)
- **Testing** subsection (`Unit:` / `Snapshots:` per [`AGENTS.md`](../../../AGENTS.md))
- **Related** / blocker issues

```sh
gh issue edit <N> --body-file /path/to/body.md
```

### 6. DEV_PLAN.md

**Only** when the issue's milestone already has a section in DEV_PLAN (see [AGENTS.md DEV_PLAN updates](../../../AGENTS.md)).

Add:

1. Row in milestone **table** (Issue | Title | Size)
2. `### [#N](...) - Title` stub under that milestone
3. Append issue number to execution-order **mermaid** list for that milestone subgraph (if listed there)
4. Add `#N` to **Relative size** examples row when representative

**Do not** add Done checkmark until a draft PR exists for implementing the issue.

**Do not** add new milestone sections or BAU issues without a DEV_PLAN milestone.

Deliver DEV_PLAN edits via a **docs PR** from `main` unless the user asks to combine with a gameplay PR.

### 7. Project stack order

Within the issue's **Status** column (usually Backlog), order by DEV_PLAN execution rank for that milestone.

**Preferred:** manual drag in GitHub project UI.

**GraphQL** (place item `<ITEM_N>` immediately after `<ITEM_AFTER>`):

```sh
gh api graphql -f query='
mutation {
  updateProjectV2ItemPosition(input: {
    projectId: "PVT_kwHOAAzqPM4BW5KP"
    itemId: "<ITEM_N>"
    afterId: "<ITEM_AFTER>"
  }) {
    clientMutationId
  }
}'
```

Reorder **within one Status column only**, not the whole board.

### 8. Status column

| Situation | Status |
|-----------|--------|
| New follow-up / blocked | **Backlog** |
| Ready for agent pickup (unblocked, prioritized) | **Agent-ready** (human or explicit user request) |
| Has open blocker | **Backlog** (not Agent-ready) |

### 9. On blocker close

When a blocker issue **closes**, comment on the blocked issue only:

```text
#<blocker> closed — #<N> unblocked (blocked-by cleared). Still Backlog until prioritized.
```

Do **not** auto-move to Agent-ready unless the user asks.

### 10. On implementation (later)

When a draft PR opens for the issue: mark DEV_PLAN section **Done** per AGENTS.md (optional PR link). Out of scope for create-time hygiene.

### 11. Hygiene comment

Post on the issue:

```markdown
## Roadmap hygiene

- **Milestone:** …
- **Size:** … | **Priority:** …
- **blocked-by:** …
- **DEV_PLAN:** row + section under …
- **Board:** Backlog, stacked after #… in m8 order

Process: [milestone-issue-hygiene](.cursor/skills/milestone-issue-hygiene/SKILL.md)
```

Adjust paths if commenting from GitHub (link to `main` tree on GitHub for the skill file).

## Humans

Contributors creating milestone issues manually should run this checklist (or ask an agent: "roadmap hygiene for #N").

## Related

- [AGENTS.md](../../../AGENTS.md) — Issue creation, DEV_PLAN updates
- [audit-github-devplan](../audit-github-devplan/SKILL.md) — reconcile drift
- [contributor-guide.md](../../../docs/contributor-guide.md) — project + DEV_PLAN overview
