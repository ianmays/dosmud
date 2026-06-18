/*
 * XP and level-up helpers. The short basename keeps the DOS tree within FAT
 * 8+3 limits while centralizing progression math.
 */

#ifndef GPROG_H
#define GPROG_H

struct GameState;
struct GameEventQueue;

/* Total XP required to complete one level-up from the given level (HUD + rules). */
int game_xp_to_next_level(int level);

int progression_enemy_xp_reward(int enemy_level, int spread_roll);
void progression_gain_xp(struct GameState *game, int amount, struct GameEventQueue *out);
void progression_gain_enemy_xp(struct GameState *game, int enemy_level,
                               int spread_roll, struct GameEventQueue *out);

#endif
