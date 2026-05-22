#ifndef WANDERER_H
#define WANDERER_H

struct GameState;

void wanderer_update_separation(struct GameState *game);
void wanderer_step(struct GameState *game);
void wanderer_begin_encounter(struct GameState *game);
void wanderer_apply_reply(int choice);
int wanderer_cmd_reply(struct GameState *game, int choice);

#endif
