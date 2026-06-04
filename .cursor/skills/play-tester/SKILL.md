---
name: play-tester
description: Run interactive LLM-driven play sessions against release dosmud, capture gitignored transcripts, and deliver plain-English reports with required blue-sky ideas. Use when the user asks to playtest or before prioritizing backlog work. Pair with the play-tester agent for judgement.
disable-model-invocation: true
---

# Play-tester

**Procedure (this skill).** Session setup, deterministic replay loop, report format, filtering.

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
- [ ] 7. Generalize findings (filter seed-only beats); write report (format below)
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
- Stuck loop (same unknown command three times) — note in report

**Parse failures:** count "Unknown command" lines; if they block play, put under **What felt rough** with plain **What** wording and quote the turn.

## What counts as feedback (filter before you write)

**Seed is for replay, not for blaming the seed.** Record seed and session paths in **Session**. Do not treat "what happened on this run" as automatically backlog-worthy.

Before adding **What felt rough** or an issue draft, ask:

*Would this confuse or disappoint someone on a different seed, or someone who already expects this fight or event?*

If **no** — put it in **Session log** only (expected scripted beat for this path).

If **unsure** — optional short second-seed probe (~10 commands) with another seed (e.g. `5678`); document both seeds in **Session**. Compare [`tests/regression/*.expect`](../../../tests/regression/) when behavior may be locked test content.

### Examples — do not elevate to "What felt rough"

| Session fact | Why skip |
|--------------|----------|
| Bandit appears when you pick up the stick at camp (seed 1234) | Deterministic beat for that path, not a defect |
| Combat starts at T:1 right after `take stick` | Expected pacing for that seed |
| Map only shows `@` until you have left camp | Player has not explored yet |

### Examples — OK as general feedback (plain player voice)

| What (headline) | Notes |
|-----------------|-------|
| After a fight I still see "the bandit is waiting" when I try to walk — I am not sure the fight is over or what to type next. | Messaging when menus block normal commands; not "bandit at camp on seed 1234" |
| Right after I won a fight, "Nobody is waiting for an answer" sounds like I did something wrong. | Copy tone; cite turn in **Seen when** |
| When a command is blocked during a fight, `help craft` told me why — more commands could do that. | Positive pattern; generalize |

### Examples — Ideas (no repro required)

- The game would be better if the first room had a short "type help" nudge before any threat.
- The game would be better if the map showed where I last fought.

Write for a **designer or product reader**, not a systems engineer. No internal jargon ("encounter state", "handover") in **What** lines or issue title candidates.

## Report format (required)

Deliver in chat:

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
- … (2–5 bullets, plain English; optional "(turn N)")

### What felt rough
(0–6 items; cap for scannability; skip if nothing generalizes)

- **What:** … (one sentence, player voice)
- **Why it matters:** …
- **Seen when:** … (optional turn or quote; *What* must still make sense without knowing the seed)

### Ideas
(required: 5–10 bullets, each starting with "The game would be better if …")

- …
- … (at least half may be aspirational; tag `[from this run]` when directly inspired by the transcript)

### If you want to track it
(optional; use the same plain **What** sentence as the issue title candidate)

- **Title:** …
- scope: …
- repro: seed + command sequence (repro lives here, not in the headline)

### Suggested actions
- [ ] file issue (draft above — not posted unless user approves)
- [ ] comment on existing issue #…
- [ ] defer / no action (default when the report is mostly Ideas)

### Session log (seed-specific, not backlog)
- … (expected beats on this seed/path only, e.g. "bandit on take stick at camp")
```

**Required sections:** Session, What felt good, Ideas.

**Optional:** What felt rough (if none, say so in one line), If you want to track it, Session log.

Do not use a category table or jargon-heavy issue titles. Omit vague praise with no player-facing effect.

## Relationship to other workflows

| Tool | Relationship |
|------|----------------|
| [testing-gap-auditor](../testing-gap-auditor/SKILL.md) | Play-tester may recommend a snapshot; test-auditor still owns `check-test-gaps.sh` |
| [milestone-issue-hygiene](../milestone-issue-hygiene/SKILL.md) | User approves before creating issues |
| [audit-github-devplan](../audit-github-devplan/SKILL.md) | Play-tester feeds backlog candidates; audit reconciles DEV_PLAN |
| [human-interventions](../human-interventions/SKILL.md) | Documents user design steering during impl, not play sessions |

## Do not

- Edit `src/` or `include/` during a playtest pass
- Open implementation PRs or move project board status
- File GitHub issues without explicit user approval
- Replace or skip `make test*` on feature branches
- List deterministic seed beats as defects in **What felt rough**

## Appendix (optional modes)

**Persona scripts:** run a fixed [`tests/regression/*.input`](../../../tests/regression/) file with `make test-run` or `./dosmud` (TEST_MODE) and interpret output; useful for regression comparison, not v1 default.

**Human-in-the-loop:** user plays in terminal; agent interviews using the same report template and records seed/path if provided.
