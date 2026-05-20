#ifndef TH_WORLD_H
#define TH_WORLD_H

struct GameState;

#ifdef TEST_MODE
/* Seed-1234 room graph shared by snapshots, unit tests, and soak. */
void harness_world_boot_graph(struct GameState *game);
#endif

#endif
