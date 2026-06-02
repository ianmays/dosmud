# dosmud Manual

This manual is the canonical project documentation set for architecture, testing workflow, and contribution process.

## Read this first

- [Architecture](architecture.md)
- [Testing](testing.md)
- [CI Metrics](ci-metrics.html)
- [Contributor Guide](contributor-guide.md)

## Roadmap and agents

- [DEV_PLAN.md](../DEV_PLAN.md) - curated roadmap log, execution order, milestone tables
- [AGENTS.md](../AGENTS.md) - coding agent workflow, Cursor rules/skills index, DEV_PLAN policy
- [GitHub project #1](https://github.com/users/ianmays/projects/1) - Status, Priority, Size, stack order

Agents: after implementation, run **code-commenter** (when `src/` changed) and **documentation** passes before a draft PR. See AGENTS.md **Comment pass** and **Documentation pass**.

## Scope

- `README.md` remains the quick-start entrypoint for repository visitors.
- This `/docs` tree is the long-form reference used for maintenance and future expansion.
- Documentation ownership table: [AGENTS.md](../AGENTS.md#documentation-ownership)

## Build/runtime note

Build command details are documented once in [Testing](testing.md) and referenced from other pages to avoid drift.
