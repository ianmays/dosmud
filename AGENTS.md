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
- ALWAYS add or update the linked issue section in [`DEV_PLAN.md`](DEV_PLAN.md) with **Done ✅** when opening a draft PR. That is a roadmap log line, not a living status tracker - do not change it on pushes or after merge.
- ALWAYS open draft PRs first.
- ALWAYS link PRs to their corresponding Issues.
- ALWAYS check whether documentation updates are required.
- ALWAYS preserve deterministic gameplay behavior.
- NEVER introduce gameplay/render/platform coupling.
- NEVER use em dash - use standard hyphen only.
- NEVER introduce C99/C11 features or compiler-specific extensions.
- After `git push` to an open PR: complete the [post-push checklist](.cursor/rules/pr-after-push.mdc) in the **same turn** before replying (see [pr-after-push skill](.cursor/skills/pr-after-push/SKILL.md)).

## GitHub Workflow (dosmud project)

### Task start checklist

1. SwitchMode -> plan
2. Select top Agent-ready issue (or confirm user-specified issue)
3. Move issue to **Planning** on project board (same turn as step 1)
4. Research and draft plan
5. Post plan comment on issue
6. Wait for user approval
7. Move to **In progress** and create branch

Plan mode forbids code and repo changes, but GitHub project status updates and issue comments are always allowed.

### Issue selection

- When asked to pick up a new issue, ALWAYS select the top issue from the `Agent-ready` column in the dosmud GitHub project.
- NEVER begin work on an issue you believe should be abandoned, deferred, or reconsidered - challenge the work instead.

### Issue creation

- If no Issue exists, ALWAYS create one.
- ALWAYS apply appropriate labels (`gameplay`, `non-functional`, `tooling`, `documentation`, etc).
- ALWAYS add the `agent` label to agent-created Issues.
- ALWAYS add new Issues to the dosmud GitHub project.

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

- ALWAYS raise draft PRs initially.
- NEVER include issue references in PR titles.
- ALWAYS ensure PR labels match Issue labels.
- ALWAYS keep PR titles and commit messages concise.
- ALWAYS use lower-case first character in PR titles and commit messages.
- NEVER use prefixes like `docs:` in PR titles or commit messages.

### After `git push` to a PR branch (mandatory)

Whenever you `git push` and the branch has an open pull request:

1. Run `gh pr view --json number,isDraft` (or check the PR on GitHub).
2. If the PR is **not a draft**: in the **same turn** as the push, **before** you finish your message to the user, run:
   `gh pr comment <number> --body "review this"`
3. Use that exact body text only. NEVER ask whether re-review is needed.

| PR state | `review this` required? |
|----------|-------------------------|
| Draft | No — skip until **Ready for review** |
| Ready for review (non-draft) | Yes — **every** push, including review fixes and docs-only commits |
| Project board **Review** + non-draft | Yes — same as ready for review |

The first push after marking the PR ready for review starts this rule; it applies to all later pushes until merge.

Do **not** skip because the PR was draft when opened. Re-check `isDraft` after **every** push. If the user marked the PR **Ready for review** since the last push, the next push requires `review this` even when earlier pushes in the session did not.

A missed `review this` comment is a **workflow failure** (same severity as skipping project status updates). Procedure: [`.cursor/rules/pr-after-push.mdc`](.cursor/rules/pr-after-push.mdc), [`.cursor/skills/pr-after-push/SKILL.md`](.cursor/skills/pr-after-push/SKILL.md), [`.cursor/rules/agent-workflow.mdc`](.cursor/rules/agent-workflow.mdc).

### Completion workflow

After merge:
- switch back to `main`
- pull latest changes
- delete completed local branches
- agents: [post-merge-cleanup skill](.cursor/skills/post-merge-cleanup/SKILL.md)

### After merge or issue closure

- Offer to document **Human Interventions** using [`.cursor/skills/human-interventions/SKILL.md`](.cursor/skills/human-interventions/SKILL.md).
- Draft first; post to the issue only after user approval or an explicit "post it" instruction.

## Technical Constraints

- Target language: ANSI C89 / ISO C90.
- Must remain compatible with both GCC and OpenWatcom.
- Prefer fixed-size arrays and static storage.
- Avoid dynamic allocation unless explicitly justified.
- Avoid recursion.
- Keep state explicit and localized.
- Avoid hidden globals and cross-module state mutation.

## Architecture Principles

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

Canonical detail: [`docs/testing.md`](docs/testing.md) (especially [When to add or update tests](docs/testing.md#when-to-add-or-update-tests)).

- **New gameplay behavior:** add or update snapshot tests when player-visible output changes; add unit tests in the owning `tests/unit/unit_<module>.c`; keep tests deterministic per `docs/testing.md`.
- **New or moved exported APIs** in coverage-scope modules listed in [`docs/testing.md`](docs/testing.md#unit-tests-greatest): at least one **direct** unit test per new function or distinct branch in the matching `tests/unit/unit_*.c` (see [When to add or update tests](docs/testing.md#when-to-add-or-update-tests)). Passing only through `game_process_input` is not enough when logic lives in a named slice API (see #90).
- **Static-only or router-only changes in `game.c`:** existing tests must pass; add router-level unit tests only when behavior or tick semantics change.
- **Render-only (`grendr`, `txtres`):** update snapshots when copy or layout changes; unit tests in those modules are **not required** for copy-only changes - add them when parsing or render state logic changes.
- **PR Test plan:** list tests added or updated, or one sentence why none (for example "docs-only PR").

Running `make test`, `make test-run`, and `make test-unit` before a PR does not replace writing tests when the above applies.

## Environment Model for DOS Flow

- `make` executes from Linux/WSL.
- `dos-prepare.ps1` executes through Windows PowerShell.
- DOSBox-X launches on the Windows side.
- `dos-prepare.local.ps1` stores machine-specific path configuration.
- `dos-prepare.ps1` copies only `src/`, `include/`, and `build.bat` into the DOS tree (per-directory `robocopy`, not a repo-root mirror); `.git` and tests never belong in the Windows tree.
- Do not assume the repository exists under `/mnt/c`.

## Documentation Ownership

- `README.md` = quick-start/operator usage
- `docs/index.md` = documentation entrypoint
- `docs/architecture.md` = subsystem boundaries and rationale
- `docs/testing.md` = deterministic testing workflow
- `docs/contributor-guide.md` = contributor and PR workflow

Prefer linking between documents over duplicating large instruction blocks.

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

## When in Doubt

- Choose the simpler implementation.
- Favor deterministic behavior.
- Favor explicit ownership boundaries.
- Prioritize DOS/OpenWatcom compatibility over convenience features.
- Prefer consistency with surrounding code over abstract "best practice".