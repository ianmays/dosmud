#ifndef WANDERER_H
#define WANDERER_H

struct GameState;
struct GameEventQueue;

/* Wanderer state is a separate roaming actor, so its movement and reply flow
 * stay isolated from the rest of the dialogue system.
 */

void wanderer_update_separation(struct GameState *game);
void wanderer_step(struct GameState *game);
void wanderer_begin_encounter(struct GameState *game, struct GameEventQueue *out);
void wanderer_apply_reply(int choice, struct GameEventQueue *out);
int wanderer_cmd_reply(struct GameState *game, int choice, struct GameEventQueue *out);

#endif
