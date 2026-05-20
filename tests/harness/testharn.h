#ifndef TESTHARN_H
#define TESTHARN_H

struct GameState;

#ifdef TEST_MODE
/*
 * Apply a test harness line (@fixture <name> or @seed <unsigned>).
 * Returns 1 if the line was a harness directive and state was updated,
 * 0 if not a harness line, -1 if the fixture name is unknown,
 * -2 if fixture setup failed (for example bag full),
 * -3 if @seed syntax is invalid.
 */
int testharn_apply(struct GameState *game, const char *line);
#endif

#endif
