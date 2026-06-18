#ifndef COMBAT_H
#define COMBAT_H

struct GameState;
struct GameEventQueue;

/* Combat owns the short enemy exchange: start, resolve a reply, then hand
 * victory or defeat back to the wider game state.
 */

int combat_player_attack_bonus(const struct GameState *game);
void combat_start(struct GameState *game, struct GameEventQueue *out);
void combat_resolve_reply(struct GameState *game, int choice, struct GameEventQueue *out);
int combat_enemy_level(const struct GameState *game);

#endif
