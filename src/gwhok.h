/*
 * Authored world advancement hooks (#220).
 * Short basename keeps the DOS tree within FAT 8+3 limits.
 */

#ifndef GWHOK_H
#define GWHOK_H

struct GameState;

/* Persisted advancement bits; save v14+. */
#define WORLD_ADV_ORCHARD_RESTORED 1
#define WORLD_ADV_TOWER_MEAL 2

/* Query persisted advancement bits in GameState.world_adv_flags. */
int gwhok_has(const struct GameState *game, int adv_id);
/* Set adv_id once, reconcile room-desc rows; returns 1 on first set, 0 if duplicate. */
int gwhok_set(struct GameState *game, int adv_id);
/*
 * Reconcile static room-desc rows from world_adv_flags. Call after load or when
 * flags change without gwhok_set (harness, tests). Does not emit gout events.
 */
void gwhok_apply_all(struct GameState *game);

#endif
