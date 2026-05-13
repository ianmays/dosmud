#ifndef PROGRESSION_H
#define PROGRESSION_H

struct GameState;

/* Total XP required to complete one level-up from the given level (HUD + rules). */
int game_xp_to_next_level(int level);

void progression_gain_xp(struct GameState *game, int amount);

#endif
