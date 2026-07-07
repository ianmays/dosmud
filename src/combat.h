#ifndef COMBAT_H
#define COMBAT_H

struct GameState;
struct GameEventQueue;

/* Who opens combat; encounter callers set this, combat applies the first strike. */
enum CombatInitiator {
    COMBAT_INITIATOR_PLAYER = 0,
    COMBAT_INITIATOR_ENEMY
};

/* Combat owns the short enemy exchange: start (with initiative), resolve a
 * reply, then hand victory or defeat back to the wider game state.
 */

int combat_player_attack_bonus(const struct GameState *game);
/* initiator: COMBAT_INITIATOR_* selects player or enemy opening strike. */
void combat_start(struct GameState *game, struct GameEventQueue *out,
                  int initiator);
void combat_resolve_reply(struct GameState *game, int choice, struct GameEventQueue *out);
/* Active encounter level from CombatState; defaults to 1 when unset. */
int combat_enemy_level(const struct GameState *game);
void combat_replay_menu(struct GameEventQueue *out);

#endif
