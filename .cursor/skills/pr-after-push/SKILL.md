---
name: pr-after-push
description: >-
  Mandatory steps after git push when the branch has an open GitHub PR:
  check isDraft and post review this on ready PRs. Use after every git push,
  before ending the agent turn.
---

# PR after push

Run this in the **same turn** as `git push`, **before** the user-facing summary. See also [`.cursor/rules/pr-after-push.mdc`](../../rules/pr-after-push.mdc) and [`AGENTS.md`](../../../AGENTS.md).

## When to run

- You ran `git push` and `gh pr view` succeeds for the current branch
- Applies to review fixes, docs-only commits, and CI follow-ups - not only the first push

## Commands

```bash
gh pr view --json number,isDraft
```

If `isDraft` is **false**:

```bash
gh pr comment <number> --body "review this"
```

Use that exact body only. Do not ask whether re-review is needed.

If `isDraft` is **true**: skip until the PR is marked **Ready for review** on GitHub.

## Decision table

| `isDraft` | `review this` |
|-----------|---------------|
| `true` | Skip |
| `false` | Required this turn |

Project board **Review** does not replace checking `isDraft`. A PR can be on the board in Review while still a GitHub draft.

## Common mistakes

| Mistake | Fix |
|---------|-----|
| End the turn with a summary right after `git push` | Run this checklist first |
| Assume the PR is still draft because it was opened as draft | Re-run `gh pr view --json number,isDraft` after every push |
| Skip because the user marked the issue Review on the project board | Use GitHub `isDraft`, not board status alone |

Example: [#136](https://github.com/ianmays/dosmud/pull/136) - review fixes were pushed after **ready for review**; `review this` was required that turn.

## PR push checklist (before replying)

- [ ] `git push` succeeded
- [ ] `gh pr view --json number,isDraft` ran this turn
- [ ] If not draft: `gh pr comment <N> --body "review this"` ran this turn
- [ ] Only then: user-facing summary
