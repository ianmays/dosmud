#ifndef NPC_H
#define NPC_H

struct GameState;
struct GameEventQueue;

/*
 * npc.h owns fixed NPC identity, roaming-NPC behavior, and room-talk lookup.
 * Dialogue and encounter slices build on these helpers instead of duplicating
 * room checks or traveler-only roaming state.
 */

int npc_room_actor(int room_id);
int npc_dialogue_actor(int dialogue_kind);
int npc_choice_is_valid(int choice);
int npc_open_room_dialogue(struct GameState *game, struct GameEventQueue *out);
void npc_seed_roaming_traveler(struct GameState *game);
void npc_roaming_update_separation(struct GameState *game);
void npc_roaming_step(struct GameState *game);
void npc_roaming_begin_encounter(struct GameState *game,
                                 struct GameEventQueue *out);
int npc_roaming_cmd_reply(struct GameState *game, int choice,
                          struct GameEventQueue *out);
/* Canonical GAME_EVENT_DIALOGUE / DIALOGUE_GUARD producers for npc-using slices. */
void npc_push_dialogue(struct GameEventQueue *out, int actor, int phase, int choice);
void npc_push_dialogue_guard(struct GameEventQueue *out, int reason);

#endif
