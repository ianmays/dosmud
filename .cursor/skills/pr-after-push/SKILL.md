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

## Resolve review threads (review fixes)

When this push addresses inline PR review comments, **resolve the matching threads on GitHub in the same turn** (after push, before the user-facing summary). Do not leave fixed threads open.

1. List threads with **path, line, and comment preview** so you can map each `id` to the fix (do not resolve by guessing):

```bash
NUM="$(gh pr view --json number -q .number)"

gh api graphql -f query='
query($owner: String!, $repo: String!, $number: Int!) {
  repository(owner: $owner, name: $repo) {
    pullRequest(number: $number) {
      reviewThreads(first: 50) {
        nodes {
          id
          isResolved
          comments(first: 1) {
            nodes { path line body }
          }
        }
      }
    }
  }
}' -f owner=ianmays -f repo=dosmud -F number="$NUM" \
  --jq '.data.repository.pullRequest.reviewThreads.nodes[]
    | {id, isResolved, path: .comments.nodes[0].path, line: .comments.nodes[0].line,
       preview: (.comments.nodes[0].body | split("\n")[0])}'
```

Resolve only rows where `isResolved` is `false` and this push addresses that `path` / comment.

2. For each matching thread, resolve (one `threadId` per call):

```bash
gh api graphql -f query='
mutation($threadId: ID!) {
  resolveReviewThread(input: { threadId: $threadId }) {
    thread { isResolved }
  }
}' -f threadId='PRRT_...'
```

Resolve only threads the commit actually addresses. If the user said "resolve threads as needed", resolve all fixed threads; if a thread still needs discussion, leave it open.

Optional: reply on the thread before resolving when the fix is non-obvious (one short line pointing at the commit).

## Common mistakes

| Mistake | Fix |
|---------|-----|
| End the turn with a summary right after `git push` | Run this checklist first |
| Assume the PR is still draft because it was opened as draft | Re-check `isDraft` this turn |
| Skip because the issue is on project board **Review** | Use GitHub `isDraft`, not board status alone |
| Skip because an earlier push this session did not need `review this` | Re-check after **Ready for review** |
| Push review fixes but leave resolved threads open on GitHub | Run **Resolve review threads** when comments were addressed |
| Resolve threads without reading path/line/preview | Use step 1 `jq` output to pick the correct `id` per fixed comment |

Example: [#136](https://github.com/ianmays/dosmud/pull/136) - review fixes after **ready for review** required `review this` that turn.

## Checklist (before replying)

- [ ] `git push` succeeded
- [ ] `gh pr view --json number,isDraft` ran this turn
- [ ] If push addressed review comments: unresolved fixed threads resolved on GitHub (or none to resolve)
- [ ] If not draft: `gh pr comment <N> --body "review this"` ran this turn
- [ ] Only then: user-facing summary
