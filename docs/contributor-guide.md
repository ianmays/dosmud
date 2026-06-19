# Contributor Guide

Thanks for contributing to dosmud.

## Development Principles

- Keep code ANSI C89 / ISO C90 compatible.
- Preserve compatibility with both GCC and OpenWatcom.
- **FAT 8.3 (8+3) names for `src/` sources and headers:** MS-DOS 5.0 through 6.22 (and our Open Watcom real-mode build) assume classic FAT volumes where the portable primary filename is still eight characters plus a three-letter extension. Longer basenames (for example `progression.h`) fail on those trees and in some DOS-hosted toolchains. Keep basenames at most eight characters (see `grendr.c`, `invent.h`, `gprog.c`, `platpos.c`, `platwin.c`, `docs/architecture.md`). The DOS build uses `build.bat` (see `docs/testing.md`): gameplay objects are archived into `gameplay.lib` via several short `wlib` lines so each COMMAND.COM invocation stays under the length limit; `TEST_MODE` adds `thwld.obj` and `tharn.obj` from `harness\` in a separate `wlib` pass (`dos-prepare` copies `tests\harness\`). When you add or remove a gameplay translation unit, update `Makefile`, `build.bat` compile lines, and the `wlib` calls that build `gameplay.lib`. Platform sources (`platdos.c` on DOS, `platpos.c` on GCC/POSIX, `platwin.c` on Windows cross-builds) link beside `main.obj`, not inside `gameplay.lib`. Build identity follows the checked-in `VERSION` file plus generated native metadata in `build/include/version.h`; DOS keeps the checked-in `include/version.h` fallback unless you deliberately refresh it in the repo.
- Keep gameplay deterministic for identical seed + inputs.
- Prefer simple, explicit, procedural code over heavy abstractions.
- Avoid unrelated refactors in the same PR.
- Keep core gameplay free of `printf` and other terminal I/O; use `render_*` in `grendr` instead (`make check-layers` allows `printf` only in `main.c`, `grendr.c`, and the platform files `platpos.c`, `platwin.c`, or `platdos.c`; `make test-all` runs the guard before the test build). Newline and spacing rules for `txtres` and `grendr` are in [architecture.md](architecture.md#newline-and-spacing).

## Pull Requests Required

Changes should be merged through Pull Requests rather than direct pushes to `main`.

Recommended workflow:

- create a focused branch
- open a draft PR early
- link relevant GitHub Issues
- keep commits small and reviewable
- update documentation when behavior or workflows change

Before opening a draft PR (agents and contributors with automation):

```text
implement → make test* → pre-draft-pr-passes (agents) or individual passes → draft PR
```

- Pre-draft PR passes (agents, default): [`.cursor/skills/pre-draft-pr-passes/SKILL.md`](https://github.com/ianmays/dosmud/blob/main/.cursor/skills/pre-draft-pr-passes/SKILL.md) orchestrates test-gap, comment, and documentation subagents (see [AGENTS.md](https://github.com/ianmays/dosmud/blob/main/AGENTS.md#pre-draft-pr-passes))
- Test-gap pass: [`.cursor/skills/testing-gap-auditor/SKILL.md`](https://github.com/ianmays/dosmud/blob/main/.cursor/skills/testing-gap-auditor/SKILL.md) (see [AGENTS.md](https://github.com/ianmays/dosmud/blob/main/AGENTS.md#testing-pass)); CI runs `scripts/check-test-gaps.sh` in informative mode (log gaps; fix before merge)
- Comment pass: [`.cursor/skills/code-commenter/SKILL.md`](https://github.com/ianmays/dosmud/blob/main/.cursor/skills/code-commenter/SKILL.md) on `src/`, `include/`, and `tests/` C sources (see [AGENTS.md](https://github.com/ianmays/dosmud/blob/main/AGENTS.md#comment-pass))
- Documentation pass: [`.cursor/skills/documentation-maintainer/SKILL.md`](https://github.com/ianmays/dosmud/blob/main/.cursor/skills/documentation-maintainer/SKILL.md) (see [AGENTS.md](https://github.com/ianmays/dosmud/blob/main/AGENTS.md#documentation-pass))

**Qualitative playtesting (optional):** agents or contributors can run interactive play sessions with the [play-tester skill](https://github.com/ianmays/dosmud/blob/main/.cursor/skills/play-tester/SKILL.md) (`make build`, `./dosmud --seed <N>`, transcripts under `playtest/sessions/` which are gitignored). Reports number **ideas** and **improvements** for easy follow-up (e.g. file issues for `ideas 4` and `improvements 2`); seed-scripted events stay in the session log, not the backlog. See [AGENTS.md](https://github.com/ianmays/dosmud/blob/main/AGENTS.md#playtesting-optional).

The repo’s GitHub project uses a **Status** field on issues: **Planning** when forming an implementation plan (add decided plan as a comment), **In progress** while you implement (before the PR exists), **Review** once the draft PR is up, **Done** after merge. Details for agents: [AGENTS.md](https://github.com/ianmays/dosmud/blob/main/AGENTS.md).

[`DEV_PLAN.md`](https://github.com/ianmays/dosmud/blob/main/DEV_PLAN.md) is a manually curated roadmap log tied to [GitHub milestones](https://github.com/ianmays/dosmud/milestones). When you open a draft **implementation** PR for an issue that already has a section here, mark **Done ✅** (optional PR link). Do not mark Done on hygiene or docs-only PRs. Issue **blocked-by** relationships on GitHub express sequencing. It is not a living status tracker (no updates on push or merge).

**New milestone issues:** set **Size** and **Priority** on [project #1](https://github.com/users/ianmays/projects/1), wire **blocked-by** on GitHub, add the DEV_PLAN table row + stub in a **docs PR** when the milestone is tracked there (agents in plan mode do GitHub steps only), and align Backlog stack order. Mark **Done ✅** in DEV_PLAN when the implementation draft PR opens, not at issue create. Agents and contributors: [`.cursor/skills/milestone-issue-hygiene/SKILL.md`](https://github.com/ianmays/dosmud/blob/main/.cursor/skills/milestone-issue-hygiene/SKILL.md). Drift audits: [`.cursor/skills/audit-github-devplan/SKILL.md`](https://github.com/ianmays/dosmud/blob/main/.cursor/skills/audit-github-devplan/SKILL.md). See [AGENTS.md](https://github.com/ianmays/dosmud/blob/main/AGENTS.md#dev_plan-updates).

### After you push

- While the PR is a **draft** on GitHub: do not post `review this` or `skip ai review`.
- After you mark the PR **Ready for review** (non-draft):
  - **First push:** always comment `review this` (exact body).
  - **Later pushes:** post `review this` when fixes are substantive (logic, tests, rework from Bugbot or an AI reviewer); post `skip ai review: <reason>` when fixes are trivial or the PR has already had multiple review passes. Agents follow the full decision tree in [pr-after-push skill](https://github.com/ianmays/dosmud/blob/main/.cursor/skills/pr-after-push/SKILL.md).
- When a push addresses inline review comments, resolve those PR review threads on GitHub (agents: same-turn step in [pr-after-push skill](https://github.com/ianmays/dosmud/blob/main/.cursor/skills/pr-after-push/SKILL.md)).
- Project board **Review** can be set when the draft PR is opened; GitHub **Ready for review** (`isDraft` false) starts the review-trigger convention, not board status alone.

Agents: policy in [AGENTS.md](https://github.com/ianmays/dosmud/blob/main/AGENTS.md#after-git-push-to-a-pr-branch-mandatory); procedure in [`.cursor/skills/pr-after-push/SKILL.md`](https://github.com/ianmays/dosmud/blob/main/.cursor/skills/pr-after-push/SKILL.md).

### CI review surfaces

Use the CI outputs for different review questions:

- the sticky PR comment shows the current pull request run in one place, including step results, coverage, soak benchmarks, and build timings
- the GitHub Actions job summary shows the same current-run stats on the workflow run page
- the [CI Metrics](ci-metrics.html) dashboard shows merged `main` history so you can check whether a timing or soak result looks like a one-off spike or part of a trend

For the workflow details behind these outputs, including how `ci-stats.json` is published and how the dashboard history is updated, see [testing.md](testing.md#ci-github-actions).

## Local Validation Before Opening a PR

Run:

```sh
make build
make check-layers
make test
make test-run
make test-unit
```

Snapshot pairs live under `tests/regression/`; unit sources under `tests/unit/` (binary and coverage output in `tests/unit/build/`, gitignored). For snapshot fixtures, roll inject, unit tests, and `quiet_explore` tick tests, see [testing.md](testing.md). When changing gameplay code, follow [When to add or update tests](testing.md#when-to-add-or-update-tests) and list test additions or updates in the PR **Test plan**.

If your changes affect build flow, DOS runtime behavior, or orchestration scripts:

```sh
make dos-prepare
make dos-run
make dos-prepare MODE=TEST_MODE
```

Recommended for broader tooling/build validation:

```sh
make build-all
make test-all
```

For the WSL -> Windows console cross-build:

```sh
make build-win
make test-win
make win-run
```

These targets default to `x86_64-w64-mingw32-gcc` and emit `dosmud.exe`. `test-win` is a compile-only `TEST_MODE` build. `win-run` launches the existing repo-root `dosmud.exe` from the most recent Windows cross-build (`make build-win` or `make test-win`) in a new Windows console window and forwards `SEED=<n>` as `--seed <n>`.

For detailed environment and workflow information, see `testing.md`.

## Commit Style

- Keep the **title** (subject line) concise; start with a lower-case character.
- Write the **body** in lower-case phrase-style lines, not sentences or paragraphs.
- **Two or more** logical changes in the body: use markdown bullets (`- `); every bullet starts lower-case.
- **Exactly one** logical change: one lower-case body line without a bullet is fine.
- Never start the body (or any bullet) with an upper-case letter.
- Squash-merge and agent drafting: [`.cursor/skills/squash-commit-message/SKILL.md`](https://github.com/ianmays/dosmud/blob/main/.cursor/skills/squash-commit-message/SKILL.md), [`.cursor/rules/commit-messages.mdc`](https://github.com/ianmays/dosmud/blob/main/.cursor/rules/commit-messages.mdc).
- Keep commits logically focused and easy to review.

## Documentation

Canonical ownership and the documentation-pass policy: [AGENTS.md](https://github.com/ianmays/dosmud/blob/main/AGENTS.md#documentation-ownership) and [AGENTS.md](https://github.com/ianmays/dosmud/blob/main/AGENTS.md#documentation-pass).

Roadmap hygiene when filing milestone issues: [milestone-issue-hygiene](https://github.com/ianmays/dosmud/blob/main/.cursor/skills/milestone-issue-hygiene/SKILL.md). Drift audits: [audit-github-devplan](https://github.com/ianmays/dosmud/blob/main/.cursor/skills/audit-github-devplan/SKILL.md).
