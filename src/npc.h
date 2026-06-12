#ifndef NPC_H
#define NPC_H

#include "base.h"

struct GameState;
struct GameEventQueue;

/*
 * npc.h owns the dynamic roster API, roaming behavior, and room-talk lookup.
 * Dialogue and encounter slices call these helpers instead of duplicating slot
 * scans or traveler-only state on GameState.
 */

int npc_room_actor(int room_id);
int npc_dialogue_actor(int dialogue_kind);
int npc_choice_is_valid(int choice);
/* Roster lifecycle: one actor per slot; slot index order is save/tick-stable. */
void npc_clear_all(struct GameState *game);
int npc_find_by_actor(const struct GameState *game, int actor);
int npc_find_by_dialogue(const struct GameState *game, int dialogue);
int npc_find_in_room(const struct GameState *game, int room_id);
int npc_spawn(struct GameState *game, int actor, int dialogue, int encounter,
              int room_id, int flags);
int npc_place(struct GameState *game, int actor, int room_id, int flags);
int npc_move(struct GameState *game, int actor, int room_id);
int npc_is_present(const struct GameState *game, int actor, int room_id);
int npc_deactivate_until(struct GameState *game, int actor, u32 return_tick);
int npc_begin_encounter(struct GameState *game, int actor, int dialogue,
                        int encounter, int room_id, int flags,
                        struct GameEventQueue *out);
int npc_end_encounter(struct GameState *game, int actor);
int npc_open_room_dialogue(struct GameState *game, struct GameEventQueue *out);
void npc_seed_roaming_traveler(struct GameState *game);
void npc_roaming_activate_due(struct GameState *game);
void npc_roaming_update_separation(struct GameState *game);
void npc_roaming_step(struct GameState *game);
void npc_roaming_begin_encounter(struct GameState *game,
                                 struct GameEventQueue *out);
int npc_roaming_begin_encounter_in_room(struct GameState *game, int room_id,
                                        struct GameEventQueue *out);
int npc_roaming_cmd_reply(struct GameState *game, int choice,
                          struct GameEventQueue *out);
/* Canonical GAME_EVENT_DIALOGUE / DIALOGUE_GUARD producers for npc-using slices. */
void npc_push_dialogue(struct GameEventQueue *out, int actor, int phase, int choice);
void npc_push_dialogue_guard(struct GameEventQueue *out, int reason);

#endif
