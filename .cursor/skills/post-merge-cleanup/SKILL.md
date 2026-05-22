---
name: post-merge-cleanup
description: >-
  Switches to main, pulls latest, and deletes merged local feature branches
  after a GitHub PR merge. Use when the user says a PR is merged, asks for
  post-merge cleanup, branch clean-up, or to perform agent completion workflow.
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

4. **Verify**

```bash
git status -sb
git branch
```

Report: current branch, sync with `origin/main`, which locals were deleted.

## Rules

| Topic | Rule |
|-------|------|
| Local delete | Use `-d` (merged). Use `-D` only if the user explicitly asks to force-delete |
| Remote delete | Do **not** run `git push origin --delete` unless the user asks |
| `main` | Never delete `main` |
| Untracked work | Switching/pulling does not remove untracked files (e.g. stashed skills) |
| Stash | Do not drop or apply the user's stash unless they ask |

## Optional follow-ups (offer, do not run unless asked)

- **Key Human Interventions:** [human-interventions skill](../human-interventions/SKILL.md) - draft only until user approves or says "post it"
- **Remote branch:** `git push origin --delete <branch>` then `git fetch --prune`

## Checklist

- [ ] On `main`, up to date with `origin/main`
- [ ] Named merged local branch(es) deleted
- [ ] User told what remains (untracked files, remote tracking branches)

## Example

User: "refactor PR is merged - perform clean-up"

```bash
git switch main
git pull
git branch -d refactor-apply-command
```

User later: "also docs-pr-after-push (merged)"

```bash
git branch -d docs-pr-after-push
```

(Already on `main` after step 1; pull only if needed.)
