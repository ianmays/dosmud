#ifndef COMBAT_H
#define COMBAT_H

struct GameState;

void combat_start(struct GameState *game);
void combat_resolve_reply(struct GameState *game, int choice);

#endif
