---
name: squash-commit-message
description: >-
  Draft GitHub squash-merge commit title and bullet body from branch commits
  for dosmud. Use when the user asks for a squash commit message, commit
  message cleanup, merge commit text, or title plus bullets from PR commits.
---

# Squash commit message (dosmud)

Produce one **title** and a **body** for squashing a PR branch. Read [`AGENTS.md`](../../AGENTS.md), [commit-messages rule](../../rules/commit-messages.mdc); this skill adds squash title `(#<pr>)` shape.

## When to run

- User asks for squash / merge commit message, "commit message clean-up", or title + bullets
- Before GitHub squash-merge, or when amending the suggested squash message on the PR

## Gather input

```bash
git log origin/main..HEAD --oneline
git diff origin/main...HEAD --stat
```

Use commit subjects and the diff to group work into a few bullets. Drop fixup-only commits (rebase noise, typo fixes) by folding them into the bullet they belong to.

## Output format

**Title** (one line):

- Lower-case first character (same as PR titles and normal commits in this repo)
- Imperative, concise, describes the whole PR
- Append PR number in parentheses: `(#134)` — use the GitHub PR number, not the issue number
- No `docs:` / `feat:` prefixes
- No issue references in the title (`Fixes #90` etc.)

**Body** (same rules as any commit; see commit-messages rule):

- **Two or more** chunks: markdown bullets (`- `), each starting lower-case
- **One** chunk only: a single lower-case line, no bullet
- Phrase-style clauses, not sentences or prose paragraphs; avoid trailing-period "essay" habit
- Optional semicolon to join two short phrases in one bullet
- **Do not** add `Closes #N`, `Fixes #N`, or issue links unless the user explicitly asks

Deliver exactly:

```markdown
**Title:**
```
<one line>
```

**Body:**
```
- <bullet>
- <bullet>
```
```

(Omit the "Closes" line by default.)

## Examples

**Example — refactor PR (#134)**

Title:
```
refactor apply_command into subsystem command handlers (#134)
```

Body:
```
- route inspect, talk, reply, and give from game.c into dialogue, genc, gatmos, and wanderer slices; apply_command stays orchestration-only (no gameplay change)
- split game_cmd_meta into session (help/quit), observe (look/map), and pass_time (wait)
- add unit tests: direct *_cmd_* APIs plus session/observe/pass_time tick behavior via game_process_input
- add branch coverage: genc reply handover/intimidate fail/invalid; dialogue talk/reply for frog, bandit, wanderer, herbalist, archivist; wanderer invalid reply
```

**Example — docs PR (#136)**

Title:
```
document when to add and update tests (#136)
```

Body:
```
- when-to-add table in docs/testing.md; testing-discipline always-on rule
- AGENTS Testing expectations and planning Testing subsection; agent-workflow PR checklist
- contributor and architecture pointers; DEV_PLAN #135 Done with PR link
```

## Checklist

- [ ] Title starts lower-case; ends with `(#<pr>)` when PR number is known
- [ ] Body is bullets (2+ items) or one lower-case line (1 item); never prose paragraphs
- [ ] No upper-case at start of body or any bullet
- [ ] No `Closes` / `Fixes` in body unless user requested
- [ ] No duplicate module tables or unrelated drive-by notes in bullets
- [ ] Bullets match the actual branch diff, not generic filler
