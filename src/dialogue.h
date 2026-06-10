#ifndef DIALOGUE_H
#define DIALOGUE_H

struct GameState;
struct GameEventQueue;

/*
 * Dialogue helpers handle fixed room-NPC reply routing on top of npc.h lookup.
 */
int dialogue_cmd_talk(struct GameState *game, struct GameEventQueue *out);
int dialogue_cmd_reply(struct GameState *game, int choice, struct GameEventQueue *out);

#endif
