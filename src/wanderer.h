#ifndef WANDERER_H
#define WANDERER_H

struct GameState;

/* Wanderer state is a separate roaming actor, so its movement and reply flow
 * stay isolated from the rest of the dialogue system.
 */

void wanderer_update_separation(struct GameState *game);
void wanderer_step(struct GameState *game);
void wanderer_begin_encounter(struct GameState *game);
void wanderer_apply_reply(int choice);
int wanderer_cmd_reply(struct GameState *game, int choice);

#endif
