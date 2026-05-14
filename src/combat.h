#ifndef COMBAT_H
#define COMBAT_H

struct GameState;

int combat_player_attack_bonus(const struct GameState *game);
void combat_start(struct GameState *game);
void combat_resolve_reply(struct GameState *game, int choice);

#endif
