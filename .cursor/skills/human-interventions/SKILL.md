---
name: human-interventions
description: >-
  Draft and post "Human Interventions" summaries on GitHub issues where the
  user steered design during agent work. Use when the user asks for human
  interventions, after an issue or PR is merged/closed, or when documenting
  design decisions on a completed ticket.
disable-model-invocation: true
---

# Human Interventions

Record where the **user** steered design during agent-led work. Post as an issue comment when they want it on the ticket.

Not a general session summary. Not workflow hygiene. Not review-bot threads the user only chased for resolution status.

## When to run

- User says: "human interventions", "document interventions", "post interventions on #N"
- After merge or issue closure: briefly **offer** this skill (see `AGENTS.md`); do not post without approval unless they already approved the draft or said "post it"

## What counts (include)

A topic is a **human intervention** only if the user changed or confirmed a **design, architecture, or test-structure** choice:

- They flagged a concern before or during implementation
- Agent presented alternatives; they chose or rejected one
- Outcome is one clear sentence

**One bullet per topic.** Use this pattern:

```markdown
**{Topic title}** — You {verb} {concern}. After discussion of {option A} versus {option B}, {chosen outcome}.
```

Use backticks for commands, paths, and symbols (`make test-soak`, `config.h`, `tests/harness/`).

## What to exclude

| Exclude | Examples |
|---------|----------|
| Workflow / process only | `review this`, board moves, plan approval, "why is this marked resolved" without engaging on the design |
| Review-bot-only | Bugbot/CodeRabbit raised it; user only chased resolution status |
| No user engagement | Agent decided alone; user never commented on that area |
| Meta hygiene | CI grep fixes, lint nits, commit style unless user explicitly steered them |

When unsure: **omit** rather than inflate the list.

## Workflow

1. **Resolve issue** — User-provided `#N`, or `gh pr view --json closingIssuesReferences` for the merged PR. Per `AGENTS.md`, do not modify closed issues except **comments** (allowed).
2. **Scan** — User messages in the current chat first; search agent transcript if the work spanned multiple turns.
3. **Filter** — Apply inclusion and exclusion rules above.
4. **Zero topics?** — Tell the user; do not post an empty comment.
5. **Draft** — Section heading `## Human Interventions`, then bullets. Show draft in chat.
6. **Approve** — Wait for approval or explicit "post it"; revise if user drops a topic (e.g. not a real intervention).
7. **Post** — Only after approval:

```bash
gh issue comment <N> --body "$(cat <<'EOF'
## Human Interventions

**Topic** — You ...
EOF
)"
```

8. **Report** — Return the issue comment URL.

## Edge cases

- **Multiple issues:** One comment per issue; ask which issue if unclear.
- **Stacked PRs:** Document the **feature issue** unless the user wants drive-by fixes called out.
- **Do not** edit an existing Human Interventions comment unless the user asks.

## Reference

Gold-standard tone and length: [examples.md](examples.md) (#116 soak work, three topics).
