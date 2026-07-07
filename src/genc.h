/*
 * Enemy encounter entry points. The short basename keeps the DOS tree FAT
 * 8+3 compatible while the logic stays separate from combat.
 */

#ifndef GENC_H
#define GENC_H

struct GameState;
struct GameEventQueue;

void enemy_begin_encounter(struct GameState *game, struct GameEventQueue *out);
int genc_cmd_reply(struct GameState *game, int choice, struct GameEventQueue *out);
int genc_cmd_give(struct GameState *game, int item_arg, struct GameEventQueue *out);
int genc_replay_active_prompt(struct GameState *game, struct GameEventQueue *out);

#endif
