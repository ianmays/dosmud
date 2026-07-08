#ifndef NPC_H
#define NPC_H

#include "base.h"

/*
 * Dialogue detail payload for the Herbalist authored slice. arg3 on TALK/REPLY;
 * GameState.herbalist_menu mirrors the active scene for reply routing in npc.c.
 */
enum HerbalistDialogueScene {
    HERBALIST_SCENE_NOT_STARTED = 0,
    HERBALIST_SCENE_REQUESTED,
    HERBALIST_SCENE_READY,
    HERBALIST_SCENE_COMPLETE,
    /* REQUESTED submenu: root keeps portrait; OPTIONS is copy-only follow-up */
    HERBALIST_SCENE_REQUESTED_OPTIONS,
    /* give/offering exchange outcomes (arg3 on GAME_EVENT_DIALOGUE). */
    HERBALIST_SCENE_GIVE_REJECTED,
    HERBALIST_SCENE_GIVE_NOT_CARRYING,
    HERBALIST_SCENE_GIVE_REWARD_BAG,
    HERBALIST_SCENE_GIVE_REWARD_GROUND,
    HERBALIST_SCENE_GIVE_REWARD_NO_SPACE
};

/*
 * Dialogue detail payload for the watchman authored slice (#8). arg3 on TALK/REPLY;
 * GameState.watchman_menu is reply routing; neutral copy variants use arg3 only.
 */
enum WatchmanDialogueScene {
    /* talk menus (TALK events) */
    WATCHMAN_SCENE_NEUTRAL = 0,
    WATCHMAN_SCENE_AFTER_WARNING,
    WATCHMAN_SCENE_MEAL_OFFER,
    WATCHMAN_SCENE_MEAL_OFFER_EMPTY,
    /* neutral root copy variants (TALK arg3 only; watchman_menu stays NEUTRAL). */
    WATCHMAN_SCENE_NEUTRAL_WARNED,
    WATCHMAN_SCENE_NEUTRAL_FED,
    WATCHMAN_SCENE_NEUTRAL_WARNED_FED,
    /* reply-only scenes (REPLY events) */
    WATCHMAN_SCENE_PECKISH,
    WATCHMAN_SCENE_WARNING,
    WATCHMAN_SCENE_CHANGE_SUBJECT,
    WATCHMAN_SCENE_SQUALL_ADVICE,
    WATCHMAN_SCENE_APOLOGY,
    WATCHMAN_SCENE_FOOD_THANKS,
    WATCHMAN_SCENE_ALREADY_FED,
    WATCHMAN_SCENE_GIVE_NOT_CARRYING,
    WATCHMAN_SCENE_GIVE_REJECTED
};

struct GameState;
struct GameEventQueue;

/*
 * npc.h owns the dynamic roster API, roaming behavior, and room-talk lookup.
 * Dialogue and encounter slices call these helpers instead of duplicating slot
 * scans or roaming-friendly dialogue state on GameState.
 */

int npc_room_actor(int room_id);
int npc_dialogue_actor(int dialogue_kind);
int npc_choice_is_valid(int choice);
/* 1 for traveler / lost animal / peddler branches (npc_roaming_cmd_reply). */
int npc_is_roaming_friendly_dialogue(int dialogue_kind);
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
/* Re-emit the current friendly dialogue or encounter menu without changing mode. */
int npc_replay_active_prompt(struct GameState *game, struct GameEventQueue *out);
/* Fixed room-NPC give/offering hook; returns 1 when a room NPC consumed it. */
int npc_cmd_give(struct GameState *game, int item_arg,
                 struct GameEventQueue *out);
/* 1 while room watchman meal-offer accepts give/offer (npc.c owns menu rules). */
int npc_watchman_give_offer_active(const struct GameState *game);
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
