# Human Interventions examples

## Issue #116 (reference)

Posted after soak/harness work merged. Three topics only; combat-interval review fix is intentionally omitted (review-bot finding, not a user design intervention).

```markdown
## Human Interventions

**Soak vs unit coverage** — You flagged that long-run stress work should not ride on `make test-unit`. After discussion of folding soak into the existing greatest binary versus a separate entry point, a dedicated `make test-soak` harness was chosen, with three fixed-seed scenarios and periodic state-ok checks.

**Performance limits** — You raised concerns about test-side file I/O (`soak_limits.txt`) and CI reading limits from a second source. Alternatives considered were a committed limits file versus compile-time ceilings aligned with other test data in C; `CFG_TEST_SOAK_LIMIT_*` in `config.h` was chosen, with each `SOAK_BENCH` line carrying `limit=` for the PR report.

**Harness layout** — You questioned keeping `testharn` under `src/` and duplicate seed-1234 world tables in `unit_util`. Options discussed were the status quo versus centralising fixture code and the world graph under `tests/harness/`; the latter was chosen (`testharn`, `th_world.c`), with `tests/regression/` remaining golden files only.
```

## Anti-pattern (do not include)

**Combat soak validation** — Bugbot noted the combat scenario used the wrong check interval; the user only asked why the thread was marked resolved. That is workflow/review hygiene, not a human intervention bullet.
