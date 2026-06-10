#ifndef NPC_H
#define NPC_H

struct GameState;
struct GameEventQueue;

/*
 * npc.h owns fixed NPC identity and room-talk lookup. Dialogue, wanderer, and
 * later NPC slices build on these helpers instead of duplicating room checks.
 */

int npc_room_actor(int room_id);
int npc_dialogue_actor(int dialogue_kind);
int npc_choice_is_valid(int choice);
int npc_open_room_dialogue(struct GameState *game, struct GameEventQueue *out);
void npc_push_dialogue(struct GameEventQueue *out, int actor, int phase, int choice);
void npc_push_dialogue_guard(struct GameEventQueue *out, int reason);

#endif
