#ifndef TESTHARN_H
#define TESTHARN_H

struct GameState;

#ifdef TEST_MODE
/*
 * Apply a test fixture line (@fixture <name>). Returns 1 if the line was a
 * fixture directive and state was updated, 0 if not a fixture line, -1 if the
 * fixture name is unknown, -2 if setup failed (for example bag full).
 */
int testharn_apply(struct GameState *game, const char *line);
#endif

#endif
