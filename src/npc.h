#ifndef NPC_H
#define NPC_H

#include "base.h"

/*
 * Dialogue detail payload for the Herbalist authored slice. This stays out of
 * GameState headers so render/copy code can consume GAME_EVENT_DIALOGUE arg3
 * without depending on full orchestration state.
 */
enum HerbalistDialogueScene {
    HERBALIST_SCENE_NOT_STARTED = 0,
    HERBALIST_SCENE_REQUESTED,
    HERBALIST_SCENE_READY,
    HERBALIST_SCENE_COMPLETE,
    /* give/offering exchange outcomes (arg3 on GAME_EVENT_DIALOGUE). */
    HERBALIST_SCENE_GIVE_REJECTED,
    HERBALIST_SCENE_GIVE_NOT_CARRYING,
    HERBALIST_SCENE_GIVE_REWARD_BAG,
    HERBALIST_SCENE_GIVE_REWARD_GROUND,
    HERBALIST_SCENE_GIVE_REWARD_NO_SPACE
};

/*
 * Dialogue detail payload for the watchman authored slice (#8). Same arg3 seam
 * as HerbalistDialogueScene; kept in npc.h for render/copy consumers.
 */
enum WatchmanDialogueScene {
    /* talk menus */
    WATCHMAN_SCENE_NEUTRAL = 0,
    WATCHMAN_SCENE_WARNED,
    /* reply outcomes from watchman_grant_herbs and neutral/warned branches */
    WATCHMAN_SCENE_HERBS_BAG,
    WATCHMAN_SCENE_HERBS_GROUND,
    WATCHMAN_SCENE_HERBS_NO_SPACE,
    WATCHMAN_SCENE_HERBS_ALREADY
};

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
/* Active roster slot for a dialogue kind (genc/combat resolve DIALOGUE_ENEMY). */
int npc_find_by_dialogue(const struct GameState *game, int dialogue);
int npc_find_in_room(const struct GameState *game, int room_id);
int npc_spawn(struct GameState *game, int actor, int dialogue, int encounter,
              int room_id, int flags);
int npc_place(struct GameState *game, int actor, int room_id, int flags);
int npc_move(struct GameState *game, int actor, int room_id);
int npc_is_present(const struct GameState *game, int actor, int room_id);
int npc_deactivate_until(struct GameState *game, int actor, u32 return_tick);
/* Dynamic encounter lifecycle: spawn slot, open dialogue, deactivate when done. */
int npc_begin_encounter(struct GameState *game, int actor, int dialogue,
                        int encounter, int room_id, int flags,
                        struct GameEventQueue *out);
int npc_end_encounter(struct GameState *game, int actor);
int npc_open_room_dialogue(struct GameState *game, struct GameEventQueue *out);
/* Reply for NPC_ROOM_INFO dialogue kinds; ignores player room after talk. */
int npc_room_cmd_reply(struct GameState *game, int choice,
                       struct GameEventQueue *out);
/* Fixed room-NPC give/offering hook; returns 1 when a room NPC consumed it. */
int npc_cmd_give(struct GameState *game, int item_arg,
                 struct GameEventQueue *out);
/* Maintain authored npc story-world hooks that do not emit events. */
void npc_story_tick(struct GameState *game);
void npc_seed_profiles(struct GameState *game);
void npc_roaming_activate_due(struct GameState *game);
void npc_roaming_update_separation(struct GameState *game);
void npc_roaming_step(struct GameState *game);
/* Open fixed encounter in room_id when an active authored slot matches; returns 1. */
int npc_fixed_begin_encounter_in_room(struct GameState *game, int room_id,
    struct GameEventQueue *out);
void npc_roaming_begin_encounter(struct GameState *game,
    struct GameEventQueue *out);
int npc_roaming_begin_encounter_in_room(struct GameState *game, int room_id,
                                        struct GameEventQueue *out);
int npc_roaming_cmd_reply(struct GameState *game, int choice,
                          struct GameEventQueue *out);
/* Canonical GAME_EVENT_DIALOGUE / DIALOGUE_GUARD producers for npc-using slices. */
void npc_push_dialogue(struct GameEventQueue *out, int actor, int phase, int choice);
/* detail is GAME_EVENT_DIALOGUE arg3 when copy needs more than actor/phase/choice. */
void npc_push_dialogue_detail(struct GameEventQueue *out, int actor, int phase,
                              int choice, int detail);
void npc_push_dialogue_guard(struct GameEventQueue *out, int reason);

#endif
