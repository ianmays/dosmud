#ifndef GOUT_H
#define GOUT_H

#include "config.h"

/*
 * Fixed-size simulation event queue. Core gameplay appends GameEvent records
 * here so command/tick stepping can run headlessly.
 * See docs/architecture.md for engine vs game-logic ownership at this seam.
 */

/*
 * Generic kinds (#47 room/move look; #157 command/nav). Slice producers use
 * game_event_push; everything else still wraps GameOutKind as GAME_EVENT_LEGACY.
 */
enum GameEventKind {
    GAME_EVENT_NONE = 0,
    GAME_EVENT_LEGACY,
    GAME_EVENT_MOVE,
    GAME_EVENT_ROOM_LOOK,
    /* #157: game.c command router emits; grendr maps to former GAME_OUT_* text. */
    GAME_EVENT_MAP,
    GAME_EVENT_HELP,           /* arg0 = command help topic (CMD_HELP_*) */
    GAME_EVENT_WAIT,
    GAME_EVENT_CANNOT_MOVE,    /* text = direction name */
    GAME_EVENT_UNKNOWN_COMMAND,
    /* #158: inventory/item slice emits generic action + outcome payloads. */
    GAME_EVENT_ITEM_RESULT,
    GAME_EVENT_BAG_VIEW,
    GAME_EVENT_CRAFT_RESULT,
    GAME_EVENT_EQUIP_RESULT
};

/*
 * Inventory/item payload contract (#158):
 * ITEM_RESULT  arg0=GameEventItemAction arg1=GameEventItemOutcome
 *              arg2=item id or ITEM_NONE arg3=value/capacity/slots when needed
 * CRAFT_RESULT arg0=crafted/attempted item id arg1=GameEventCraftOutcome
 * EQUIP_RESULT arg0=item id or ITEM_NONE arg1=GameEventEquipOutcome
 */

enum GameEventItemAction {
    GAME_ITEM_ACTION_NONE = 0,
    GAME_ITEM_ACTION_LOOT,
    GAME_ITEM_ACTION_TAKE,
    GAME_ITEM_ACTION_DROP,
    GAME_ITEM_ACTION_EAT,
    GAME_ITEM_ACTION_USE
};

enum GameEventItemOutcome {
    GAME_ITEM_OUTCOME_NONE = 0,
    GAME_ITEM_OUTCOME_OK,
    GAME_ITEM_OUTCOME_BLOCKED_COMBAT,
    GAME_ITEM_OUTCOME_NO_BODY,
    GAME_ITEM_OUTCOME_BODY_STRIPPED,
    GAME_ITEM_OUTCOME_BAG_FULL_DROP,
    GAME_ITEM_OUTCOME_NOTHING_HERE,
    GAME_ITEM_OUTCOME_NOT_HERE,
    GAME_ITEM_OUTCOME_BAG_FULL,
    GAME_ITEM_OUTCOME_NOT_CARRYING,
    GAME_ITEM_OUTCOME_GROUND_FULL,
    GAME_ITEM_OUTCOME_WRONG_ITEM,
    GAME_ITEM_OUTCOME_HP_FULL
};

enum GameEventCraftOutcome {
    GAME_CRAFT_OUTCOME_NONE = 0,
    GAME_CRAFT_OUTCOME_OK,
    GAME_CRAFT_OUTCOME_BLOCKED_COMBAT,
    GAME_CRAFT_OUTCOME_NEED_INGREDIENTS,
    GAME_CRAFT_OUTCOME_UNKNOWN
};

enum GameEventEquipOutcome {
    GAME_EQUIP_OUTCOME_NONE = 0,
    GAME_EQUIP_OUTCOME_ALREADY_WIELDING,
    GAME_EQUIP_OUTCOME_NOT_CARRYING,
    GAME_EQUIP_OUTCOME_NOT_WEAPON,
    GAME_EQUIP_OUTCOME_STOW_FAIL,
    GAME_EQUIP_OUTCOME_WIELDED,
    GAME_EQUIP_OUTCOME_UNWIELD_EMPTY,
    GAME_EQUIP_OUTCOME_UNWIELD_STOWED,
    GAME_EQUIP_OUTCOME_UNWIELD_CANNOT,
    GAME_EQUIP_OUTCOME_UNWIELD_DROPPED
};

/*
 * Legacy presentation kinds (GAME_EVENT_LEGACY + legacy_kind). Command/nav
 * MAP, HELP, WAIT, CANNOT_MOVE, UNKNOWN_COMMAND, and MOVED also have
 * GAME_EVENT_* kinds after #157; keep these until all gout_push callers migrate.
 */
enum GameOutKind {
    GAME_OUT_NONE = 0,
    GAME_OUT_ROOM_LOOK,
    GAME_OUT_MAP,
    GAME_OUT_HELP,
    GAME_OUT_BANDIT_ENCOUNTER_OPEN,
    GAME_OUT_COMBAT_START,
    GAME_OUT_COMBAT_ENEMY_STRIKE,
    GAME_OUT_COMBAT_PLAYER_FALLEN,
    GAME_OUT_COMBAT_STATUS_LINE,
    GAME_OUT_COMBAT_PLAYER_HIT,
    GAME_OUT_COMBAT_BRACED,
    GAME_OUT_COMBAT_NO_SALVE_BAG,
    GAME_OUT_COMBAT_SALVE_IN_COMBAT,
    GAME_OUT_COMBAT_SALVE_FULL,
    GAME_OUT_COMBAT_INVALID_CHOICE,
    GAME_OUT_COMBAT_BANDIT_DEFEATED,
    GAME_OUT_COMBAT_MENU,
    GAME_OUT_XP_GAINED,
    GAME_OUT_LEVEL_UP,
    GAME_OUT_NEARBY_ITEM_NOTICE,
    GAME_OUT_ANIMAL_NOISE_LINE,
    GAME_OUT_ATMOSPHERE_GUST,
    GAME_OUT_ATMOSPHERE_RUSTLE,
    GAME_OUT_ATMOSPHERE_BERRY_DROP,
    GAME_OUT_ATMOSPHERE_CREAK,
    GAME_OUT_ATMOSPHERE_WATER,
    GAME_OUT_ATMOSPHERE_REED_DROP,
    GAME_OUT_ATMOSPHERE_GRIT,
    GAME_OUT_WANDERER_SCENE,
    GAME_OUT_WANDERER_REPLY,
    GAME_OUT_FROG_DIALOGUE_INTRO,
    GAME_OUT_FROG_DIALOGUE_BRANCH,
    GAME_OUT_MSG_BANDIT_WAITING_REPLY,
    GAME_OUT_MSG_BANDIT_WAITING_HANDOVER_PICK,
    GAME_OUT_BANDIT_HANDOVER_PICK_PROMPT,
    GAME_OUT_MSG_BANDIT_GIVE_NOT_CARRYING,
    GAME_OUT_MSG_GIVE_WRONG_CONTEXT,
    GAME_OUT_MSG_UNKNOWN_COMMAND,
    GAME_OUT_MSG_WAIT,
    GAME_OUT_MSG_CANNOT_MOVE,
    GAME_OUT_MSG_MOVED,
    GAME_OUT_MSG_INSPECT_NOTHING,
    GAME_OUT_MSG_INSPECT_WRONG_FOCUS,
    GAME_OUT_MSG_INSPECT_RUSTLE,
    GAME_OUT_MSG_INSPECT_CREAK,
    GAME_OUT_MSG_INSPECT_WATER,
    GAME_OUT_MSG_INSPECT_GRIT,
    GAME_OUT_MSG_BANDIT_BLOCKS_TALK,
    GAME_OUT_MSG_TRAVELER_WAITING,
    GAME_OUT_MSG_WATCHMAN_TALK,
    GAME_OUT_MSG_HERBALIST_TALK,
    GAME_OUT_MSG_ARCHIVIST_TALK,
    GAME_OUT_MSG_NOBODY_TALK,
    GAME_OUT_MSG_WATCHMAN_REPLY,
    GAME_OUT_MSG_HERBALIST_REPLY,
    GAME_OUT_MSG_ARCHIVIST_REPLY,
    GAME_OUT_MSG_HAND_OVER_ITEM,
    GAME_OUT_MSG_BAG_EMPTY_BANDIT,
    GAME_OUT_MSG_INTIMIDATE_SUCCESS,
    GAME_OUT_MSG_INTIMIDATE_FAIL,
    GAME_OUT_MSG_PICK_123,
    GAME_OUT_MSG_NOBODY_WAITING_REPLY,
    GAME_OUT_INV_NO_BODY_LOOT,
    GAME_OUT_INV_BODY_STRIPPED,
    GAME_OUT_INV_BAG_FULL_DROP,
    GAME_OUT_INV_LOOT,
    GAME_OUT_INV_NO_RUMMAGE_COMBAT,
    GAME_OUT_INV_TAKE_NOTHING,
    GAME_OUT_INV_CANNOT_TAKE_HERE,
    GAME_OUT_INV_BAG_FULL,
    GAME_OUT_INV_PICKUP,
    GAME_OUT_INV_NO_DROP_COMBAT,
    GAME_OUT_INV_NOT_CARRYING,
    GAME_OUT_INV_GROUND_FULL,
    GAME_OUT_INV_DROP,
    GAME_OUT_INV_BAG,
    GAME_OUT_INV_NO_EAT_COMBAT,
    GAME_OUT_INV_CANNOT_EAT,
    GAME_OUT_INV_EAT_BERRY_HEALED,
    GAME_OUT_INV_EAT_BERRY_FULL,
    GAME_OUT_INV_EAT_FISH_HEALED,
    GAME_OUT_INV_EAT_FISH_FULL,
    GAME_OUT_INV_USE_REPLY_COMBAT,
    GAME_OUT_INV_USE_TORCH,
    GAME_OUT_INV_USE_SALVE,
    GAME_OUT_INV_USE_SALVE_FULL,
    GAME_OUT_INV_USE_SPEAR,
    GAME_OUT_INV_NO_USE,
    GAME_OUT_INV_NO_CRAFT_COMBAT,
    GAME_OUT_INV_NEED_TORCH,
    GAME_OUT_INV_CRAFT_TORCH,
    GAME_OUT_INV_NEED_SALVE,
    GAME_OUT_INV_CRAFT_SALVE,
    GAME_OUT_INV_NEED_SPEAR,
    GAME_OUT_INV_CRAFT_SPEAR,
    GAME_OUT_INV_CRAFT_UNKNOWN,
    GAME_OUT_INV_ALREADY_WIELDING,
    GAME_OUT_INV_WIELD_NOT_WEAPON,
    GAME_OUT_INV_WIELD_STOW_FAIL,
    GAME_OUT_INV_WIELD,
    GAME_OUT_INV_UNWIELD_EMPTY,
    GAME_OUT_INV_UNWIELD,
    GAME_OUT_INV_UNWIELD_CANNOT,
    GAME_OUT_INV_UNWIELD_GROUND
};

struct GameOutEvent {
    int kind;         /* GameEventKind, or GAME_EVENT_LEGACY for old producers */
    int legacy_kind;  /* GameOutKind when kind is GAME_EVENT_LEGACY */
    int arg0;
    int arg1;
    int arg2;
    int arg3;
    int room_id;
    int room_item[CFG_AREA_ITEM_SLOTS];
    const char *text;
};

struct GameOutput {
    struct GameOutEvent events[CFG_GAME_OUT_MAX];
    int count;
    int overflowed;
};

typedef struct GameOutEvent GameEvent;
typedef struct GameOutput GameEventQueue;

void game_event_queue_reset(GameEventQueue *out);
GameEvent *game_event_push(GameEventQueue *out, int kind, int arg0, int arg1,
                           int arg2, int arg3, const char *text);
GameEvent *game_event_push_legacy(GameEventQueue *out, int legacy_kind,
                                  int arg0, int arg1, int arg2, int arg3,
                                  const char *text);

/* Transitional helpers for legacy GAME_OUT_* producers. */
void gout_reset(struct GameOutput *out);
int gout_push(struct GameOutput *out, int kind, int arg0, int arg1, int arg2,
              int arg3, const char *text);

#endif
