/*
 * Enemy encounter entry points. The short basename keeps the DOS tree FAT
 * 8+3 compatible while the logic stays separate from combat.
 */

#ifndef GENC_H
#define GENC_H

struct GameState;

void enemy_begin_encounter(struct GameState *game);
int genc_cmd_reply(struct GameState *game, int choice);
int genc_cmd_give(struct GameState *game, int item_arg);

#endif
