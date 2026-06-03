# AGENTS.md

Guidance for AI/code agents working in this repository.

## Purpose

- Keep changes aligned with DOS-first goals and deterministic ANSI C design.
- Preserve fast local iteration on Linux while validating DOS/OpenWatcom compatibility.
- Avoid architectural drift and documentation duplication.

## Core Agent Rules (Non-Negotiable)

- ALWAYS start in plan mode - then IMMEDIATELY move the linked Issue to **Planning** (same response, before issue research or plan drafting)
- ALWAYS check for an existing GitHub Issue before starting work.
- NEVER modify or reopen Closed Issues.
- ALWAYS create a branch before making changes.
- Update [`DEV_PLAN.md`](DEV_PLAN.md) per **DEV_PLAN updates** below when opening a draft PR. That file is a roadmap log, not a living status tracker - do not change it on pushes or after merge.
- ALWAYS open draft PRs first.
- ALWAYS link PRs to their corresponding Issues.
- ALWAYS complete a **documentation pass** before opening a draft PR (see [Documentation pass](#documentation-pass)).
- ALWAYS preserve deterministic gameplay behavior.
- NEVER introduce gameplay/render/platform coupling.
- NEVER use em dash - use standard hyphen only.
- NEVER introduce C99/C11 features or compiler-specific extensions.
- After behavioral implementation: run **test-gap**, **code-commenter** (when `src/` / `include/` changed), and **documentation** passes before draft PR (see [Testing pass](#testing-pass), [Comment pass](#comment-pass), [Documentation pass](#documentation-pass)).
- After `git push` to an open PR: complete the [post-push gate](#after-git-push-to-a-pr-branch-mandatory) in the **same turn** before replying.

## GitHub Workflow (dosmud project)

Task-start and PR gates: [`.cursor/rules/agent-workflow.mdc`](.cursor/rules/agent-workflow.mdc) (always applied; this file is the long-form reference).

### Task start checklist

1. SwitchMode -> plan
2. Select top Agent-ready issue (or confirm user-specified issue)
3. Move issue to **Planning** on project board (same turn as step 1)
4. Research and draft plan
5. Post plan comment on issue
6. Wait for user approval
7. Move to **In progress** and create branch

Plan mode forbids code and repo changes, but GitHub project status updates and issue comments are always allowed. [Milestone issue hygiene](.cursor/skills/milestone-issue-hygiene/SKILL.md) in plan mode is **GitHub-only** (defer `DEV_PLAN.md` to a docs PR after approval).

### Issue selection

- When asked to pick up a new issue, ALWAYS select the top issue from the `Agent-ready` column in the dosmud GitHub project (procedure: [find-next-agent-ready-task](.codex/skills/find-next-agent-ready-task/SKILL.md)).
- NEVER begin work on an issue you believe should be abandoned, deferred, or reconsidered - challenge the work instead.

### Issue creation

- If no Issue exists, ALWAYS create one.
- ALWAYS apply appropriate labels (`gameplay`, `non-functional`, `tooling`, `documentation`, etc).
- ALWAYS add the `agent` label to agent-created Issues.
- ALWAYS add new Issues to the dosmud GitHub project.
- If the issue has a **Milestone** listed in [`DEV_PLAN.md`](DEV_PLAN.md) (milestone index): run [milestone-issue-hygiene](.cursor/skills/milestone-issue-hygiene/SKILL.md) in the **same turn** for GitHub metadata (Size, Priority, blocked-by, issue body scope/Testing, project stack order, hygiene comment). Add DEV_PLAN table row + stub via a **docs PR** when execution is allowed; in **plan mode**, defer `DEV_PLAN.md` commits (GitHub-only hygiene). Do **not** skip DEV_PLAN for milestone-tracked issues once a branch is allowed.

### Status transitions

Do not skip workflow stages.

- **Planning** - set in the same turn as `SwitchMode`, before issue research or plan drafting. Never defer until plan approval.
- **In progress** - set before implementation work starts.
- **Review** - set when the draft PR is opened.
- **Done** - set only after merge and Issue closure.

### Planning expectations

- ALWAYS add agreed implementation details as an Issue comment before moving to `In progress`.
- If a PR already exists, ALWAYS include updating the PR in the implementation plan.
- ALWAYS include a **Testing** subsection in the plan (issue comment or plan doc):

```markdown
### Testing
- Unit: <files / test names or "none - reason">
- Snapshots: <files or "none - reason">
```

See [Testing expectations](#testing-expectations) and [`docs/testing.md`](docs/testing.md#when-to-add-or-update-tests).


### PR expectations

#### Testing pass

After behavioral implementation and `make test` / `make test-run` / `make test-unit` (or `make test-all`), run a **test-gap** audit before the code-commenter and documentation passes.

| Layer | Role |
|-------|------|
| [testing-gap-after-implement rule](.cursor/rules/testing-gap-after-implement.mdc) | Gate before draft PR |
| [testing-gap-auditor skill](.cursor/skills/testing-gap-auditor/SKILL.md) | Checklist, obligation matrix |
| [test-auditor agent](.cursor/agents/test-auditor.md) | Delegate full pass; may add tests |

Run `sh scripts/check-test-gaps.sh origin/main` (exit 0 required locally before draft PR). CI runs the same script in **informative** mode (logs gaps, does not fail the job).

Order: `implement → make test (and related targets) → test-gap pass → code-commenter pass (if src/ or include/ touched) → documentation pass → draft PR`.

Skip when the script passes with no gameplay/test diff, the user opts out, or the branch diff is unchanged since a completed test-gap pass this session.

#### Comment pass

After behavioral implementation, `make test*` targets, and the test-gap pass, delegate a **code-commenter** pass on translation units touched by the branch diff (vs `main`) before the documentation pass and draft PR.

- Rule: [`.cursor/rules/code-commenter-after-implement.mdc`](.cursor/rules/code-commenter-after-implement.mdc)
- Subagent (judgement): [`.cursor/agents/code-commenter.md`](.cursor/agents/code-commenter.md)
- Skill (procedure): [`.cursor/skills/code-commenter/SKILL.md`](.cursor/skills/code-commenter/SKILL.md)

Skip when the change is docs/tooling-only with no `src/` or `include/` edits, the user opts out, or the branch diff vs `main` is unchanged since a code-commenter pass completed this session (re-run after commits that change the diff). Comment-only edits in that pass; do not change executable behavior.

#### Documentation pass

After behavioral implementation, tests, and any code-commenter pass, run a **documentation pass** on the branch diff (vs `main`) before opening a draft PR.

| Layer | Role |
|-------|------|
| [documentation-discipline rule](.cursor/rules/documentation-discipline.mdc) | Always-on reminder; points to gate |
| [documentation-after-implement rule](.cursor/rules/documentation-after-implement.mdc) | Gate before draft PR |
| [documentation-maintainer skill](.cursor/skills/documentation-maintainer/SKILL.md) | Checklist and output format |
| [docs-steward agent](.cursor/agents/docs-steward.md) | Judgement; what to update or delegate |
| [milestone-issue-hygiene skill](.cursor/skills/milestone-issue-hygiene/SKILL.md) | Milestone issue create/groom (GitHub + DEV_PLAN when committing) |
| [audit-github-devplan skill](.cursor/skills/audit-github-devplan/SKILL.md) | Roadmap drift audit (fix only if user asks) |

Order: `implement → make test (and related targets) → test-gap pass → code-commenter pass (if src/ or include/ touched) → documentation pass → draft PR`.

Skip when the user opts out, the branch diff vs `main` is unchanged since a documentation pass completed this session (re-run after commits that change the diff), or the change truly has no doc impact (confirm in summary). Plan mode: defer `DEV_PLAN.md` commits per milestone hygiene skill.

#### Documentation ownership

| Path | Role |
|------|------|
| `README.md` | quick-start / operator usage |
| `docs/index.md` | documentation entrypoint |
| `docs/architecture.md` | subsystem boundaries and rationale |
| `docs/testing.md` | deterministic testing and build workflow |
| `docs/contributor-guide.md` | contributor and PR workflow |
| `AGENTS.md` | agent workflow, Cursor index, DEV_PLAN policy |
| `DEV_PLAN.md` | curated roadmap log (see **DEV_PLAN updates** below) |

Prefer linking between documents over duplicating large instruction blocks.

#### DEV_PLAN updates

[`DEV_PLAN.md`](DEV_PLAN.md) is a manually curated roadmap log aligned with GitHub milestones, not a catalog of every GitHub issue.

Before editing for a draft PR, search `DEV_PLAN.md` for the issue (e.g. `#90` or its section heading):

| Situation | Action |
|-----------|--------|
| Issue **already has a section** (implementation draft PR) | Mark that section **Done ✅** (optional PR link). |
| New issue with a **Milestone** already represented in DEV_PLAN (milestone heading and issue table) | Add table row + `### [#N](...)` stub via [milestone-issue-hygiene](.cursor/skills/milestone-issue-hygiene/SKILL.md) **docs PR** at create time; mark **Done ✅** only when a draft **implementation** PR opens. |
| Otherwise (no section; no Milestone; or milestone not in DEV_PLAN) | **Do not** edit DEV_PLAN. Track on the project board and issue/PR. |

New BAU issues without a Milestone typically fall in the last row - do not add them.

Check Milestone: `gh issue view <N> --json milestone,title`.

Do not update DEV_PLAN on later pushes or after merge.

When auditing milestone alignment, execution order, blocked-by links, or project board Priority vs stack order, use [`.cursor/skills/audit-github-devplan/SKILL.md`](.cursor/skills/audit-github-devplan/SKILL.md).

- ALWAYS raise draft PRs initially.
- NEVER include issue references in PR titles.
- ALWAYS ensure PR labels match Issue labels.
- ALWAYS keep PR titles and commit messages concise.
- ALWAYS use lower-case first character in PR titles and commit message bodies (first line and every bullet).
- ALWAYS write commit bodies as lower-case phrase-style bullets when there are two or more logical items; use a single lower-case line (no bullet) only when there is exactly one item. Do not write commit bodies as sentences or prose paragraphs.
- NEVER use prefixes like `docs:` in PR titles or commit messages.
- Commit message procedure: [`.cursor/rules/commit-messages.mdc`](.cursor/rules/commit-messages.mdc), [`.cursor/skills/squash-commit-message/SKILL.md`](.cursor/skills/squash-commit-message/SKILL.md).

### After `git push` to a PR branch (mandatory)

Complete the post-push gate in the **same turn** as `git push`, **before** the user-facing summary, whenever the branch has an open pull request.

| Layer | Role |
|-------|------|
| [pr-after-push rule](.cursor/rules/pr-after-push.mdc) | Same-turn gate; workflow failure if skipped |
| [pr-after-push skill](.cursor/skills/pr-after-push/SKILL.md) | Commands, pitfalls, checklist |
| This section | Policy - when `review this` is required |

#### Policy

| PR state | `review this` required? |
|----------|-------------------------|
| Draft (`isDraft` true) | No - skip until **Ready for review** on GitHub |
| Ready for review (non-draft) | Yes - **every** push, including review fixes and docs-only commits |
| Project board **Review** + non-draft | Yes - same as ready for review; board status does not replace `isDraft` |

The first push after **Ready for review** starts the requirement; it applies to every later push until merge.

Do **not** skip because the PR was opened as draft, because board status is **Review**, or because an earlier push this session did not need `review this`. Re-check `isDraft` after **every** push.

Comment body must be exactly `review this`. Do not ask whether re-review is needed.

A missed `review this` on a non-draft PR is a **workflow failure** (same severity as skipping project status updates). See also [agent-workflow.mdc](.cursor/rules/agent-workflow.mdc) PR push checklist.

### Completion workflow

After merge:
- switch back to `main`
- pull latest changes
- delete completed local branches
- agents: [post-merge-cleanup skill](.cursor/skills/post-merge-cleanup/SKILL.md)

### After merge or issue closure

- Offer to document **Key Human Interventions** using [`.cursor/skills/human-interventions/SKILL.md`](.cursor/skills/human-interventions/SKILL.md).
- Draft first; post to the issue only after user approval or an explicit "post it" instruction.

## Technical Constraints

See [`.cursor/rules/c89-portability.mdc`](.cursor/rules/c89-portability.mdc) for portability detail (including FAT 8.3 basenames under `src/`).

- Target language: ANSI C89 / ISO C90.
- Must remain compatible with both GCC and OpenWatcom.
- Use FAT 8.3 basenames for new `src/` `.c` / `.h` files (at most eight characters before the extension).
- Prefer fixed-size arrays and static storage.
- Avoid dynamic allocation unless explicitly justified.
- Avoid recursion.
- Keep state explicit and localized.
- Avoid hidden globals and cross-module state mutation.

## Architecture Principles

Always-applied rules: [architecture-boundaries](.cursor/rules/architecture-boundaries.mdc), [subsystem-ownership](.cursor/rules/subsystem-ownership.mdc). Rationale: [`docs/architecture.md`](docs/architecture.md).

- Keep gameplay deterministic for identical seed + inputs.
- Keep simulation, rendering, and platform concerns separated.
- Keep `game.c` orchestration-focused.
- Avoid re-centralizing unrelated systems into `game.c`.
- Prefer simple, procedural, explicit control flow.
- Avoid heavy abstraction patterns.
- Do not introduce ECS-like frameworks or object-emulation architectures.
- Favor explicit ownership boundaries over shared mutable systems.

## Validation Workflow

Primary local validation loop:

```sh
make build
make check-layers
make test
make test-run
make test-unit
```

Cross-path validation when touching build/runtime behavior:

```sh
make build-all
make test-all
```

DOS workflow:

```sh
make dos-prepare
make dos-run
```

Deterministic DOS mode:

```sh
make dos-prepare MODE=TEST_MODE
```

## Testing expectations

Canonical detail: [`docs/testing.md`](docs/testing.md) (especially [When to add or update tests](docs/testing.md#when-to-add-or-update-tests)). Authoring and pre-PR runs: [`.cursor/rules/testing-discipline.mdc`](.cursor/rules/testing-discipline.mdc).

- **New gameplay behavior:** add or update snapshot tests when player-visible output changes; add unit tests in the owning `tests/unit/unit_<module>.c`; keep tests deterministic per `docs/testing.md`.
- **New or moved exported APIs** in coverage-scope modules listed in [`docs/testing.md`](docs/testing.md#unit-tests-greatest): at least one **direct** unit test per new function or distinct branch in the matching `tests/unit/unit_*.c` (see [When to add or update tests](docs/testing.md#when-to-add-or-update-tests)). Passing only through `game_process_input` is not enough when logic lives in a named slice API (see #90).
- **Static-only or router-only changes in `game.c`:** existing tests must pass; add router-level unit tests only when behavior or tick semantics change.
- **Render-only (`grendr`, `txtres`):** update snapshots when copy or layout changes; unit tests in those modules are **not required** for copy-only changes - add them when parsing or render state logic changes.
- **PR Test plan:** list tests added or updated, or one sentence why none (for example "docs-only PR").

Running `make test`, `make test-run`, and `make test-unit` before a PR does not replace writing tests when the above applies.

## Environment Model for DOS Flow

- `make` executes from Linux/WSL.
- `dos-prepare.ps1` executes through Windows PowerShell.
- DOS launches on the Windows side.
- `dos-prepare.local.ps1` stores machine-specific path configuration.
- `dos-prepare.ps1` copies only `src/`, `include/`, and `build.bat` into the DOS tree (per-directory `robocopy`, not a repo-root mirror); `.git` and tests never belong in the Windows tree.
- Do not assume the repository exists under `/mnt/c`.

## Editing and Change Hygiene

- Make focused changes.
- Avoid unrelated opportunistic refactors.
- Preserve surrounding naming/style/layout conventions unless explicitly asked.
- Verify build and tooling documentation against real scripts and targets.
- Prefer small, reviewable commits with clear intent.
- Follow [Testing expectations](#testing-expectations) for new gameplay, new exported APIs, and refactors that move logic into slice modules.

## Cursor Cloud Instructions

Expected environment:
- Ubuntu-based Linux VM
- `gcc`
- `make`

Supported validation loop:

```sh
make build
make check-layers
make test
make test-run
make test-unit
```

Additional notes:
- `make check-layers` rejects `printf` in core gameplay files.
- `make test` builds with `-Werror -DTEST_MODE`.
- `make test-run` executes deterministic snapshot tests under `tests/regression/`.
- `make test-unit` builds `tests/unit/build/dosmud_unit` (greatest harness; see `docs/testing.md`).
- DOS validation targets are unavailable in cloud VMs.
- No lint tooling exists beyond GCC warning enforcement.
- NEVER include co-author statements in commits.

## Cursor configuration

Index of project Cursor rules, skills, and agents. Several rules are **always applied** at edit time; skills are invoked for specific workflows.

### Rules (`.cursor/rules/`)

| Rule | Role |
|------|------|
| [agent-workflow](.cursor/rules/agent-workflow.mdc) | Task start, plan mode, before PR/push |
| [architecture-boundaries](.cursor/rules/architecture-boundaries.mdc) | Module focus; avoid ECS/framework drift |
| [subsystem-ownership](.cursor/rules/subsystem-ownership.mdc) | Owning module; core vs render/platform |
| [c89-portability](.cursor/rules/c89-portability.mdc) | C89, FAT 8.3, OpenWatcom/GCC |
| [testing-discipline](.cursor/rules/testing-discipline.mdc) | When to author tests; pre-PR `make test*` |
| [testing-gap-after-implement](.cursor/rules/testing-gap-after-implement.mdc) | Test-gap audit before draft PR |
| [documentation-discipline](.cursor/rules/documentation-discipline.mdc) | Docs alignment reminder |
| [documentation-after-implement](.cursor/rules/documentation-after-implement.mdc) | Documentation pass before draft PR |
| [commit-messages](.cursor/rules/commit-messages.mdc) | PR titles and commit body style |
| [pr-after-push](.cursor/rules/pr-after-push.mdc) | Same-turn post-push gate |
| [code-commenter-after-implement](.cursor/rules/code-commenter-after-implement.mdc) | Comment pass after behavioral impl |

### Skills (`.cursor/skills/`)

| Skill | When |
|-------|------|
| [milestone-issue-hygiene](.cursor/skills/milestone-issue-hygiene/SKILL.md) | New or groomed milestone-tracked issues |
| [audit-github-devplan](.cursor/skills/audit-github-devplan/SKILL.md) | Roadmap/board/DEV_PLAN alignment audit |
| [code-commenter](.cursor/skills/code-commenter/SKILL.md) | Comment pass procedure and summary format |
| [testing-gap-auditor](.cursor/skills/testing-gap-auditor/SKILL.md) | Unit/snapshot gap audit before draft PR |
| [documentation-maintainer](.cursor/skills/documentation-maintainer/SKILL.md) | Documentation pass checklist |
| [squash-commit-message](.cursor/skills/squash-commit-message/SKILL.md) | Squash-merge title and bullets |
| [pr-after-push](.cursor/skills/pr-after-push/SKILL.md) | Post-push commands and checklist |
| [post-merge-cleanup](.cursor/skills/post-merge-cleanup/SKILL.md) | After merge: branch cleanup |
| [human-interventions](.cursor/skills/human-interventions/SKILL.md) | Key Human Interventions draft (user approval) |

### Codex skills (`.codex/skills/`)

| Skill | When |
|-------|------|
| [find-next-agent-ready-task](.codex/skills/find-next-agent-ready-task/SKILL.md) | Next task from project board **Agent-ready** column |

### Agents (`.cursor/agents/`)

| Agent | When |
|-------|------|
| [code-commenter](.cursor/agents/code-commenter.md) | Delegate full comment pass (judgement); pair with skill above |
| [test-auditor](.cursor/agents/test-auditor.md) | Delegate test-gap pass; pair with skill above |
| [docs-steward](.cursor/agents/docs-steward.md) | Delegate full documentation pass; pair with skill above |

## When in Doubt

- Choose the simpler implementation.
- Favor deterministic behavior.
- Favor explicit ownership boundaries.
- Prioritize DOS/OpenWatcom compatibility over convenience features.
- Prefer consistency with surrounding code over abstract "best practice".