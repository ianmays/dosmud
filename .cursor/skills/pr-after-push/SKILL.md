---
name: pr-after-push
description: >-
  Run after every git push when the branch has an open PR: gh pr view isDraft,
  then review this on non-draft PRs, before the agent turn ends. Policy in
  AGENTS.md; same-turn gate in pr-after-push.mdc rule.
---

# PR after push

**Procedure (this skill).** Commands, pitfalls, and checklist below.

**Policy (canonical).** When `review this` is required: [`AGENTS.md`](../../../AGENTS.md) section **After `git push` to a PR branch**.

**Gate (rule).** Same-turn blocker: [`.cursor/rules/pr-after-push.mdc`](../../rules/pr-after-push.mdc).

## When to run

- `git push` succeeded and `gh pr view` works for the current branch
- Review fixes, docs-only commits, and CI follow-ups - not only the first push

## Commands

```bash
gh pr view --json number,isDraft
```

If `isDraft` is **false**:

```bash
gh pr comment <number> --body "review this"
```

Use that exact body only. Do not ask whether re-review is needed.

If `isDraft` is **true**: skip until the PR is **Ready for review** on GitHub.

Re-run `gh pr view --json number,isDraft` after **every** push; never infer draft state from an earlier turn or project board **Review**.

## Common mistakes

| Mistake | Fix |
|---------|-----|
| End the turn with a summary right after `git push` | Run this checklist first |
| Assume the PR is still draft because it was opened as draft | Re-check `isDraft` this turn |
| Skip because the issue is on project board **Review** | Use GitHub `isDraft`, not board status alone |
| Skip because an earlier push this session did not need `review this` | Re-check after **Ready for review** |

Example: [#136](https://github.com/ianmays/dosmud/pull/136) - review fixes after **ready for review** required `review this` that turn.

## Checklist (before replying)

- [ ] `git push` succeeded
- [ ] `gh pr view --json number,isDraft` ran this turn
- [ ] If not draft: `gh pr comment <N> --body "review this"` ran this turn
- [ ] Only then: user-facing summary
