---
name: documentation-maintainer
description: >-
  Procedure for keeping dosmud docs, AGENTS.md, README, and DEV_PLAN aligned
  with code and Cursor config after implementation. Use before draft PR, when
  architecture or workflows change, or when the user asks for a documentation
  pass. Pairs with docs-steward agent for judgement.
---

# Documentation maintainer

**Procedure (this skill).** Checklist keyed off branch diff vs `main` (or user paths).

**Policy (canonical).** [`AGENTS.md`](../../../AGENTS.md) **Documentation pass** and **Documentation ownership**.

**Judgement (agent).** [`.cursor/agents/docs-steward.md`](../../agents/docs-steward.md) when delegating a full pass.

**Gate (rules).** [documentation-discipline.mdc](../../rules/documentation-discipline.mdc), [documentation-after-implement.mdc](../../rules/documentation-after-implement.mdc).

## When to run

1. After behavioral implementation, tests, and test-gap pass, before opening a draft PR.
2. User asks for a documentation review or sweep on named paths.
3. New or changed Cursor rule, skill, or agent (update AGENTS **Cursor configuration**).
4. Milestone issue create/groom (delegate [milestone-issue-hygiene](../milestone-issue-hygiene/SKILL.md)).
5. Roadmap drift audit (delegate [audit-github-devplan](../audit-github-devplan/SKILL.md); fix only if user asks).

## Scope

- **Inventory:** `git diff main...HEAD` over the **whole branch** to detect change signals (`src/`, `include/`, tests, Makefile, `.cursor/`, etc.).
- **Edits:** limit to documentation paths (`docs/`, `AGENTS.md`, `README.md`, `DEV_PLAN.md`, `.cursor/` rules/skills/agents). Do not rewrite gameplay code in this pass.
- Prefer links over duplicating AGENTS or other skill bodies.

## Checklist by change signal

Copy and track:

```text
Documentation pass:
- [ ] 1. Diff inventory (what changed vs main)
- [ ] 2. Map signals to doc targets (table below)
- [ ] 3. Edit or skip with reason
- [ ] 4. Roadmap skills if applicable (hygiene / audit)
- [ ] 5. Output summary (format below)
```

| Change signal | Likely targets | Delegate |
|---------------|----------------|----------|
| `src/` or `include/` subsystem, seam, exported API | [`docs/architecture.md`](../../../docs/architecture.md) | [code-commenter](../code-commenter/SKILL.md) comment-only pass on `src/` and `include/` |
| Tests, Makefile, `make` targets | [`docs/testing.md`](../../../docs/testing.md) | [testing-gap-auditor](../testing-gap-auditor/SKILL.md) if gaps remain |
| PR, GitHub, agent workflow | [`docs/contributor-guide.md`](../../../docs/contributor-guide.md), [`AGENTS.md`](../../../AGENTS.md) | - |
| New/changed `.cursor/` rule, skill, agent | [`AGENTS.md`](../../../AGENTS.md) Cursor configuration table | - |
| Quick-start, clone, run | [`README.md`](../../../README.md) | - |
| Milestone-tracked issue created/groomed | `DEV_PLAN.md` row + stub (docs PR when allowed) | [milestone-issue-hygiene](../milestone-issue-hygiene/SKILL.md) |
| Roadmap / board / DEV_PLAN drift | Report; doc-only fixes in pass | [audit-github-devplan](../audit-github-devplan/SKILL.md) |
| Draft **implementation** PR opening | DEV_PLAN **Done** checkmark per AGENTS | AGENTS **DEV_PLAN updates** (not full audit) |

## Conventions

- No new TODO comments in docs without an existing issue number.
- Temporary behavior: document intent; cite filed follow-up issue when one exists.
- Do not edit closed GitHub issues.
- DEV_PLAN: do not add BAU issues without milestone; **Done** only on implementation draft PR.
- Plan mode: GitHub-only for milestone hygiene; defer `DEV_PLAN.md` commits until a branch is allowed.

## Workflow

1. `git diff main...HEAD --stat` over the full branch (or user paths) to inventory signals; then open doc targets only when warranted.
2. Walk checklist rows that apply; open each target doc only when the diff warrants it.
3. Verify links and Makefile/`make` targets if testing or build docs touched.
4. If milestone issue work: run hygiene skill steps (GitHub + DEV_PLAN when committing).
5. If user asked for audit: run audit skill report; apply doc fixes they approve.

## Output format

```markdown
### Files updated
- ...

### Documentation added or changed
- `path`: what was aligned

### Skipped
- `path` or area: one-line reason

### Delegated
- skill or agent: why

### Risks
- only if docs may drift soon (e.g. active migration)
```

## Out of scope unless asked

- Gameplay or test logic changes.
- Repo-wide comment sweeps (use code-commenter).
- GitHub board moves or metadata without user approval.
- Bulk audit fixes without user approval after audit report.
