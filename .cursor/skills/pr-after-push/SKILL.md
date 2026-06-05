---
name: pr-after-push
description: >-
  Run after every git push when the branch has an open PR: re-check isDraft,
  run the review-this decision tree, resolve fixed threads, then post review
  this or skip ai review before the agent turn ends. Policy in AGENTS.md;
  same-turn gate in pr-after-push.mdc rule.
---

# PR after push

**Procedure (this skill).** Commands, decision tree, pitfalls, and checklist below.

**Policy (canonical).** When `review this` is required: [`AGENTS.md`](../../../AGENTS.md) section **After `git push` to a PR branch**.

**Gate (rule).** Same-turn blocker: [`.cursor/rules/pr-after-push.mdc`](../../rules/pr-after-push.mdc).

## When to run

- `git push` succeeded and `gh pr view` works for the current branch
- Review fixes, docs-only commits, and CI follow-ups - not only the first push

Re-run `gh pr view --json number,isDraft` after **every** push; never infer draft state from an earlier turn or project board **Review**.

## Review-this decision tree

After push, if `isDraft` is **true**: skip all review triggers until the PR is **Ready for review** on GitHub.

If `isDraft` is **false**, gather context (below), then apply layers in order:

### Layer 1 - always trigger

**First push** after **Ready for review** (no prior exact-body `review this` comment on the PR): **always** post `review this`. Stop here.

### Layer 2 - reviewer-driven re-review

When this push addresses reviewer feedback (resolved inline threads and/or a prior human `CHANGES_REQUESTED` review), post `review this` if the fix is **substantive**:

**Substantive** (bias toward `review this`):

- logic or behaviour change in `src/` / `include/` (not comment-only)
- new or materially changed unit/snapshot test expectations
- Bugbot or AI reviewer feedback that required rework, not a one-line tweak
- multiple related files changed to satisfy one review thread
- agent is uncertain about fix correctness

**Non-substantive** (bias toward skip + rationale comment):

- documentation or copy brought in line with review - judged by fix nature, not file path
- comment-only or naming nit in code
- CI/config glue with no gameplay semantics change
- typo or formatting called out by reviewer

### Layer 3 - maturity dampening

Count prior **`review this` comments** on the PR (exact body match only).

| Pass count | Default bias |
|------------|--------------|
| 0 | Layer 1 (first ready push) |
| 1 | when uncertain, **review** |
| 2+ | when fixes are final tweaks, **skip** unless Layer 2 says substantive |
| 3+ | strong bias to **skip** unless human `CHANGES_REQUESTED` or clear logic rework |

### Layer 4 - push without reviewer feedback

Non-draft pushes **not** responding to open review threads (small fix, CI follow-up): default **skip** `review this` + post skip rationale. Exception: user explicitly asked for re-review in chat or on the PR.

Bugbot still runs on push automatically; this tree only gates the **`review this`-triggered AI review** path.

## Gather context

```bash
gh pr view --json number,isDraft,comments \
  --jq '{number, isDraft, reviewThisCount: ([.comments[].body] | map(select(. == "review this")) | length)}'

gh pr view --json reviews \
  --jq '[.reviews[] | {author: .author.login, state, submittedAt}] | sort_by(.submittedAt)'

git diff --stat origin/main...HEAD
git log -1 --oneline
```

Read resolved thread `path` + `preview` (graphql below) to classify substantive vs trivial. Do not use file-path rules alone.

## Post review this

When the decision tree says review:

```bash
gh pr comment <number> --body "review this"
```

Use that exact body only. Do not ask whether re-review is needed.

## Post skip rationale

When the decision tree says skip on a non-draft PR:

```bash
gh pr comment <number> --body "skip ai review: <one-line reason>"
```

Examples:

- `skip ai review: doc alignment from ai reviewer thread; no logic change`
- `skip ai review: pass 3; final comment-only tweaks; all threads resolved`
- `skip ai review: ci follow-up only; no reviewer feedback addressed`

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
| Post `review this` on every non-draft push | Run the decision tree; skip trivial or mature fixes |
| Skip without posting `skip ai review: ...` on a non-draft PR | Always post skip rationale when not reviewing |
| Post `review this` on pass 3 for comment-only tweaks | Apply Layer 3 maturity dampening |
| Push review fixes but leave resolved threads open on GitHub | Run **Resolve review threads** when comments were addressed |
| Resolve threads without reading path/line/preview | Use step 1 `jq` output to pick the correct `id` per fixed comment |

Example: [#136](https://github.com/ianmays/dosmud/pull/136) - substantive review fixes after **ready for review** required `review this` that turn.

## Checklist (before replying)

- [ ] `git push` succeeded
- [ ] `gh pr view --json number,isDraft` ran this turn
- [ ] If push addressed review comments: unresolved fixed threads resolved on GitHub (or none to resolve)
- [ ] If not draft: decision tree ran; `review this` or `skip ai review: ...` posted this turn
- [ ] Only then: user-facing summary
