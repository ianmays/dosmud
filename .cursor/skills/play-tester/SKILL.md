---
name: play-tester
description: Run interactive LLM play sessions against release dosmud; deliver plain-English reports with numbered ideas and improvements, varied idea phrasing, and issue filing by number (e.g. ideas 4, improvements 2). Pair with the play-tester agent.
disable-model-invocation: true
---

# Play-tester

**Procedure (this skill).** Session setup, deterministic replay loop, report format, filtering, issue-by-number workflow.

**Judgement (agent).** [`.cursor/agents/play-tester.md`](../../agents/play-tester.md) when delegating a full session.

**Policy.** Not a PR gate; does not replace `make test*`, test-auditor, or soak. Does not file GitHub issues without user approval.

## When to run

- User: "playtest main", "playtest before #158", "play the game and gather feedback"
- Optionally **offer** after a milestone merge (do not auto-run from rules alone)
- **Not** during implementation PR CI or test-gap passes

## Inputs (ask or infer)

| Input | Default |
|-------|---------|
| `seed` | `1234` |
| `budget` | 40 commands (stop at `quit` or budget) |
| `focus` | optional area (inventory, combat, exploration, dialogue) |
| `persona` | optional hint: `new_player`, `combat`, `completionist` |

## Build and binary

Use the **release** build (not `make test` unless probing fixtures):

```sh
make build
./dosmud --seed <N>   # always pass --seed for reproducibility
```

**Optional TEST_MODE path:** `make test` then `./dosmud --seed <N>` with `@fixture` lines in the session file (see [`tests/harness/testharn.c`](../../../tests/harness/testharn.c)). Default remains release build + `--seed`.

**Platform:** Linux/WSL primary. DOS/OpenWatcom play is manual unless the user runs it.

## Session files (gitignored)

```sh
mkdir -p playtest/sessions
SEED=1234
SESSION="playtest/sessions/$(date +%Y%m%d)-seed-${SEED}.input"
OUT="${SESSION%.input}.output"
: > "$SESSION"
```

- Append **one command per line** (no `#` comment lines in v1; the game does not treat them as harness directives in release build).
- Re-run the **full** script after each batch of new lines:

```sh
./dosmud --seed "$SEED" < "$SESSION" > "$OUT" 2>&1
```

Full replay from line 1 keeps determinism for a fixed seed (same model as snapshot `.input` files).

### Stdin and replay (important)

When input is a file, the main loop reads lines until **EOF**, then exits (often printing `bye`). A script with only one command processes that command and then quits.

For agent-driven play:

- **Do not** append a single line and re-run expecting another interactive turn; EOF ends the session.
- **Do** grow the script in **batches** (several commands per append) or build the full script through `quit`, then re-run the whole file and read `OUT`.
- End the script with `quit` when you want a clean stop before budget is spent.

## Turn loop (interactive LLM)

Copy and track:

```text
Playtest session:
- [ ] 1. make build
- [ ] 2. Create SESSION / OUT paths; record seed in report
- [ ] 3. Append commands (batch or full script); include quit when done
- [ ] 4. Run ./dosmud --seed N < SESSION > OUT
- [ ] 5. Read tail of OUT (HUD, room, recent > lines, combat/menu text)
- [ ] 6. Choose next commands; append; repeat 4-5 until quit, budget, or stuck
- [ ] 7. Generalize findings (filter seed-only beats); write numbered report (format below)
```

**Choosing commands:**

- Prefer real player verbs from `help` output ([`src/command.c`](../../../src/command.c) parsing).
- **new_player persona:** early turns: `help`, `look`, `map`, then `move` / `take` as exits allow.
- **combat persona:** seek fights, use `reply` in combat, salve/bag when hurt.
- **completionist:** loot, bag, craft, wield when items appear.
- Use **active** commands (`wait`, `move`, …); do not rely on idle background ticks for v1 (release build may advance time on empty stdin polls).

**End session when:**

- `quit` issued (or `bye` path reached)
- Command budget reached
- Stuck loop (same unknown command three times) - note in report

**Parse failures:** count "Unknown command" lines; if they block play, add a numbered **Improvements** item with plain **What** wording and quote the turn.

## What counts as feedback (filter before you write)

**Seed is for replay, not for blaming the seed.** Record seed and session paths in **Session**. Do not treat "what happened on this run" as automatically backlog-worthy.

Before adding an **Improvements** item, ask:

*Would this confuse or disappoint someone on a different seed, or someone who already expects this fight or event?*

If **no** - put it in **Session log** only (expected scripted beat for this path).

If **unsure** - optional short second-seed probe (~10 commands) with another seed (e.g. `5678`); document both seeds in **Session**. Compare [`tests/regression/*.expect`](../../../tests/regression/) when behavior may be locked test content.

### Examples - do not elevate to Improvements

| Session fact | Why skip |
|--------------|----------|
| Bandit appears when you pick up the stick at camp (seed 1234) | Deterministic beat for that path, not a defect |
| Combat starts at T:1 right after `take stick` | Expected pacing for that seed |
| Map only shows `@` until you have left camp | Player has not explored yet |

### Examples - OK as Improvements (numbered in report)

| What (one sentence) | Notes |
|---------------------|-------|
| After a fight I still see "the bandit is waiting" when I try to walk - I am not sure the fight is over or what to type next. | Messaging when menus block normal commands |
| Right after I won a fight, "Nobody is waiting for an answer" sounds like I did something wrong. | Copy tone; cite turn in **Seen when** |

### Examples - Ideas (numbered, varied phrasing)

```markdown
### Ideas
Blue-sky ideas - pick numbers if you want issues filed (e.g. "ideas 4, 7").

1. Show a short "type help" nudge in the first room before any threat.
2. Players always know when a fight is really over.
3. First `map` could print a tiny legend (@ = you).
```

- **Ideas count (agent + skill):** numbered **1-10** (hard cap 10). Deliver **at least 5** when the session had enough exploration (~25+ commands or multiple rooms); fewer only for very short smokes, with a one-line reason in **Session**.
- At most **two** lines per report may use "would be better if"; prefer imperative or outcome sentences.
- **Forbidden:** every idea starting with the same phrase.
- Use ASCII hyphens only in play-tester docs and reports (no em dash or en dash per AGENTS.md).

Write for a **designer or product reader**, not a systems engineer. No internal jargon ("encounter state", "handover") in improvement **What** lines or issue title candidates.

## Report format (required)

Deliver in chat. Use **stable numbering** so the user can say e.g. *file issues for ideas 4, 7 and improvements 2, 5*.

```markdown
## Playtest report

### Session
- seed: … (optional second seed if probed: …)
- binary: ./dosmud (release build)
- commands: N
- persona / focus: …
- session files: playtest/sessions/… (local, gitignored)
- note: this run is replayable; findings below are meant to generalize beyond this seed unless noted

### What felt good
- … (2-5 unnumbered bullets, plain English; optional "(turn N)")

### Improvements (what felt rough)
_None - nothing generalized beyond this seed,_ **or:**

1. **What:** … (one plain sentence, player voice)
   **Why:** …
   **Seen when:** … (optional; *What* must make sense without knowing the seed)

2. **What:** …
   …
(up to 6 numbered items)

### Ideas
Blue-sky ideas - pick numbers if you want issues filed (e.g. "ideas 4, 7").

1. …
2. …
… (numbered 1-10, hard cap 10; aim for at least 5 when exploration was sufficient; one short sentence each; vary phrasing; at most two may use "would be better if"; tag `[from this run]` when transcript-inspired)

### How to act on this report
- Reference items by number: **ideas 4, 7** and **improvements 2, 5** (singular *idea* / *improvement* also OK).
- Say **"file issues for …"** or **"post it"** - agent drafts GitHub issues from those numbers (improvement **What** or idea line as title; repro in body for improvements).
- **Defer** is fine when you only wanted the list; do not pre-list full issue drafts in the report.

### Session log (seed-specific, not backlog)
- … (expected beats on this seed/path only)
```

**Required sections:** Session, What felt good, Ideas (numbered 1-10 per rules above), How to act on this report.

**Optional:** Improvements (or one line that none generalized), Session log.

Do not use a category table or jargon-heavy issue titles. Do not pre-fill **If you want to track it** or checkbox **Suggested actions** blocks in the default report.

## Issue filing by number (after the report)

When the user cites numbers (see agent), draft issues on demand - do not post without approval.

| Source | Title | Body |
|--------|-------|------|
| Improvement `N` | improvement `N` **What** sentence (lower-case first char) | scope bullet; repro: seed + commands from **Seen when** / session |
| Idea `N` | idea line `N` text (trimmed; lower-case first char) | scope bullet; repro optional unless user asks |

### After approval (mandatory GitHub metadata)

Per [`AGENTS.md`](../../../AGENTS.md) **Issue creation**, for each issue created from playtest follow-up in the **same turn**:

1. `gh issue create` (or create with labels in one step) using the approved title and body.
2. Domain label: `gameplay`, `tooling`, `documentation`, etc. (match the item; player-facing feedback is usually `gameplay`).
3. `agent` label on every agent-created issue.
4. `gh project item-add 1 --owner ianmays --url https://github.com/ianmays/dosmud/issues/<N>` unless the issue is already on project #1.
5. If the issue has a **Milestone** in [`DEV_PLAN.md`](../../../DEV_PLAN.md) milestone index: run [milestone-issue-hygiene](../milestone-issue-hygiene/SKILL.md) create-time checklist in the same turn (not optional). Otherwise labels + project only.

## Relationship to other workflows

| Tool | Relationship |
|------|----------------|
| [testing-gap-auditor](../testing-gap-auditor/SKILL.md) | Play-tester may recommend a snapshot; test-auditor still owns `check-test-gaps.sh` |
| [milestone-issue-hygiene](../milestone-issue-hygiene/SKILL.md) | Required same-turn pass when filing issues with a DEV_PLAN-tracked milestone |
| [audit-github-devplan](../audit-github-devplan/SKILL.md) | Play-tester feeds backlog candidates; audit reconciles DEV_PLAN |
| [human-interventions](../human-interventions/SKILL.md) | Documents user design steering during impl, not play sessions |

## Do not

- Edit `src/` or `include/` during a playtest pass
- Open implementation PRs or move project board status
- File GitHub issues without explicit user approval
- Create playtest follow-up issues without domain label, `agent` label, and project #1 entry
- Replace or skip `make test*` on feature branches
- List deterministic seed beats under **Improvements**
- Repeat the same opening phrase on every numbered idea
- Use em dash or en dash in play-tester docs or reports

## Appendix (optional modes)

**Persona scripts:** run a fixed [`tests/regression/*.input`](../../../tests/regression/) file with `make test-run` or `./dosmud` (TEST_MODE) and interpret output; useful for regression comparison, not v1 default.

**Human-in-the-loop:** user plays in terminal; agent interviews using the same report template and records seed/path if provided.
