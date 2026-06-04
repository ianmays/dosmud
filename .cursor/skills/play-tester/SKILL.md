---
name: play-tester
description: >-
  Run interactive LLM-driven play sessions against the release dosmud binary,
  capture gitignored session transcripts, and produce structured roadmap feedback.
  Use when the user asks to playtest, before prioritizing backlog work, or after
  a milestone merge (offer only). Pair with the play-tester agent for judgement.
disable-model-invocation: true
---

# Play-tester

**Procedure (this skill).** Session setup, deterministic replay loop, report format.

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
- Re-run the **full** script every turn:

```sh
./dosmud --seed "$SEED" < "$SESSION" > "$OUT" 2>&1
```

Full replay from line 1 keeps determinism for a fixed seed (same model as snapshot `.input` files).

## Turn loop (interactive LLM)

Copy and track:

```text
Playtest session:
- [ ] 1. make build
- [ ] 2. Create SESSION / OUT paths; record seed in report
- [ ] 3. Run ./dosmud --seed N < SESSION > OUT
- [ ] 4. Read tail of OUT (HUD, room, recent > lines, combat/menu text)
- [ ] 5. Choose one valid command; append to SESSION
- [ ] 6. Repeat until quit, game ends, or budget exhausted
- [ ] 7. Write playtest report (format below)
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

**Parse failures:** count "Unknown command" lines; quote the offending turn in the report.

## Report format (required)

Deliver in chat:

```markdown
## Playtest report

### Session
- seed: …
- binary: ./dosmud (release build)
- commands: N
- persona / focus: …
- session files: playtest/sessions/… (local, gitignored)

### Observations
| # | Category | Observation | Evidence (turn / output) | Roadmap suggestion |

Categories: UX, clarity, pacing, balance-feel, dead-end, bug-suspect, positive

### Suggested actions
- [ ] file issue (draft below — not posted unless user approves)
- [ ] comment on existing issue #…
- [ ] defer / no action

### Issue draft (optional)
<title one line, lower-case first character per repo commit style>

- bullet scope
- bullet repro (seed + command sequence)
```

Each observation needs **evidence** (turn number or quoted output). Omit vague praise or nitpicks without repro.

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

## Appendix (optional modes)

**Persona scripts:** run a fixed [`tests/regression/*.input`](../../../tests/regression/) file with `make test-run` or `./dosmud` (TEST_MODE) and interpret output; useful for regression comparison, not v1 default.

**Human-in-the-loop:** user plays in terminal; agent interviews using the same report template and records seed/path if provided.
