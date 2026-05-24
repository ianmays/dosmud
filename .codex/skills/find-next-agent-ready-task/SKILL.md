---
name: find-next-agent-ready-task
description: Find the next agent-ready dosmud issue from GitHub project board 1, move it to Planning, and report the selected task. Use when the user asks for the next task, the top Agent-ready issue, or to hand off the next issue for work.
---

# Find Next Agent Ready Task

## Workflow

1. Query project 1 for items with `status:"Agent-ready"`.
2. Take the first returned issue as the next task.
3. If the list is empty, report that no Agent-ready issue exists and stop.
4. Move the selected issue to `Planning` before any deeper research or planning.
5. Report the issue number, title, and URL.

## Rules

- Use `gh project item-list` as the source of truth.
- Do not infer order from issue number, milestone, or labels.
- Do not skip the project-board handoff when the user wants the next task.
- Keep the selection deterministic: use the first returned Agent-ready item.

## Handoff

- Prefer a brief result message with the chosen issue number, title, and URL.
- If the board query fails, report the `gh` error directly and do not guess.
