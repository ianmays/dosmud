---
name: post-merge-cleanup
description: >-
  After a merged PR: update main, delete local feature branches, set the closing
  issue to Done on project #1 with the card at the top of the Done column, then
  report. Use when the user says a PR is merged or asks for post-merge cleanup.
disable-model-invocation: true
---

# Post-merge cleanup (dosmud)

Run when a PR is **merged** and the user wants local hygiene (or says "perform the clean-up" / "as per agent instructions"). See [`AGENTS.md`](../../../AGENTS.md) **Completion workflow**.

## When to run

- User reports a PR merged on GitHub
- User asks to clean up branches after merge
- User names one or more merged branch names to remove

Do **not** run on draft PRs or open PRs still in review.

## Workflow

1. **Confirm branch name(s)**  
   - From the user message, or `git branch --show-current` if still on the feature branch  
   - Optional: `gh pr view <number> --json headRefName,state` to verify `MERGED`

2. **Update `main`**

```bash
git switch main
git pull
```

3. **Delete local feature branch(es)**

```bash
git branch -d <branch>
```

Repeat for each merged branch the user named (e.g. `refactor-apply-command`, `docs-pr-after-push`).

4. **Verify (git)**

```bash
git status -sb
git branch
```

5. **Project board: Done + top of Done column**

Keeps **most recently merged** issues at the **top** of the **Done** column (not at the bottom). GitHub-only; no repo commits.

Resolve the merged PR number from the user message or `gh pr list --state merged --limit 5`.

**5a. Closing issue(s)**

```bash
gh pr view <PR> --json closingIssuesReferences,state
```

If `closingIssuesReferences` is empty: skip board steps for that PR; note in the report.

**5b. Project item id** (per issue number)

```bash
gh project item-list 1 --owner ianmays --format json --limit 200 \
  -q '.items[] | select(.content.number == <N>) | .id'
```

If no item: report "issue #N not on project #1". Do not `item-add` unless the user asks (see [milestone-issue-hygiene](../milestone-issue-hygiene/SKILL.md) step 1).

**5c. Status = Done** (idempotent)

Verify field/option ids if the board layout changed:

```bash
gh project field-list 1 --owner ianmays --format json \
  -q '.fields[] | select(.name=="Status") | .options[] | "\(.name) \(.id)"'
```

```bash
gh project item-edit --id <ITEM_ID> --project-id PVT_kwHOAAzqPM4BW5KP \
  --field-id PVTSSF_lAHOAAzqPM4BW5KPzhSJ38o \
  --single-select-option-id 98236657
```

(`98236657` = Done as of 2026-06; re-resolve from step above if edits fail.)

**5d. Move card to top of Done stack**

Omit `afterId` so the item is first in project order (top of the Done grouping in the board UI). Same mutation as [milestone-issue-hygiene](../milestone-issue-hygiene/SKILL.md) section 7; post-merge always uses top placement.

```bash
gh api graphql -f query='
mutation {
  updateProjectV2ItemPosition(input: {
    projectId: "PVT_kwHOAAzqPM4BW5KP"
    itemId: "<ITEM_ID>"
  }) {
    clientMutationId
  }
}'
```

**Multiple closing issues:** repeat 5b-5d for each. Move each to top in sequence so the primary issue (last moved) ends on top, or move only the issue the user names.

Do **not** reorder Backlog, Agent-ready, or other Status columns in this pass.

6. **Report**

Include:

- Git: current branch, sync with `origin/main`, deleted locals
- Board: issue number(s), item id, Status set to Done (yes/no), moved to top (yes/no), skips (no linked issue, not on project)

## Rules

| Topic | Rule |
|-------|------|
| Local delete | Use `-d` (merged). Use `-D` only if the user explicitly asks to force-delete |
| Remote delete | Do **not** run `git push origin --delete` unless the user asks |
| `main` | Never delete `main` |
| Untracked work | Switching/pulling does not remove untracked files (e.g. stashed skills) |
| Stash | Do not drop or apply the user's stash unless they ask |
| Issue body | Do **not** edit closed issue bodies (AGENTS); board metadata only |
| Status **Done** | Matches AGENTS: **Done** after merge and issue closure |
| Board scope | **Done** column stack only; no full-roadmap reorder |

## Optional follow-ups (offer, do not run unless asked)

- **Key Human Interventions:** [human-interventions skill](../human-interventions/SKILL.md) - draft only until user approves or says "post it"
- **Remote branch:** `git push origin --delete <branch>` then `git fetch --prune`
- **Roadmap reorder:** [audit-github-devplan](../audit-github-devplan/SKILL.md) - not part of every merge

## Checklist

- [ ] On `main`, up to date with `origin/main`
- [ ] Named merged local branch(es) deleted
- [ ] Closing issue(s) identified from merged PR
- [ ] Project card Status = **Done**
- [ ] Card at **top** of Done column
- [ ] User told git + board results (including any skips)

## Example

User: "refactor PR is merged - perform clean-up"

```bash
git switch main
git pull
git branch -d refactor-apply-command
gh pr view <PR> --json closingIssuesReferences
# resolve ITEM_ID, item-edit Status Done, updateProjectV2ItemPosition (no afterId)
```

User later: "also docs-pr-after-push (merged)"

```bash
git branch -d docs-pr-after-push
# repeat board steps for that PR's closing issue if different
```

(Already on `main` after step 2; pull only if needed.)
