# dosmud Manual

This manual is the canonical project documentation set for architecture, testing workflow, and contribution process.

## Read this first

- [Architecture](architecture.md)
- [Testing](testing.md)
- [CI Metrics](ci-metrics.html)
- [Contributor Guide](contributor-guide.md)
- GitHub Releases follow the tagged draft-release flow documented in [Testing](testing.md#ci-github-actions) and [Contributor Guide](contributor-guide.md)

## Roadmap and agents

- [DEV_PLAN.md](https://github.com/ianmays/dosmud/blob/main/DEV_PLAN.md) - curated roadmap log, execution order, milestone tables
- [AGENTS.md](https://github.com/ianmays/dosmud/blob/main/AGENTS.md) - coding agent workflow, Cursor rules/skills index, DEV_PLAN policy
- [GitHub project #1](https://github.com/users/ianmays/projects/1) - Status, Priority, Size, stack order

Agents: after implementation, run **code-commenter** (when `src/`, `include/`, or `tests/` C sources changed) and **documentation** passes before a draft PR. See [AGENTS.md](https://github.com/ianmays/dosmud/blob/main/AGENTS.md#comment-pass) and [AGENTS.md](https://github.com/ianmays/dosmud/blob/main/AGENTS.md#documentation-pass).

## Scope

- `README.md` remains the quick-start entrypoint for repository visitors.
- This `/docs` tree is the long-form reference used for maintenance and future expansion.
- Documentation ownership table: [AGENTS.md](https://github.com/ianmays/dosmud/blob/main/AGENTS.md#documentation-ownership)

## Build/runtime note

Build command details are documented once in [Testing](testing.md) and referenced from other pages to avoid drift.
