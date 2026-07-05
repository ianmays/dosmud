#ifndef GOUT_H
#define GOUT_H

#include "config.h"

/*
 * Fixed-size simulation event queue. Core gameplay appends GameEvent records
 * here so command/tick stepping can run headlessly.
 * See docs/architecture.md for engine vs game-logic ownership at this seam.
 */

/*
 * Generic kinds (#47 room/move look; #157 command/nav; #158 invent; #159 combat;
 * #160 dialogue/encounter; #161 ambient/inspect). Producers use game_event_push.
 */
enum GameEventKind {
    GAME_EVENT_NONE = 0,
    GAME_EVENT_MOVE,
    GAME_EVENT_ROOM_LOOK,
    /* #157: game.c command router emits; grendr maps to player-visible text. */
    GAME_EVENT_MAP,
    GAME_EVENT_HELP,           /* arg0 = command help topic (CMD_HELP_*) */
    GAME_EVENT_VERSION,        /* #202: text = build identity line from buildid */
    GAME_EVENT_WAIT,
    GAME_EVENT_CANNOT_MOVE,    /* text = direction name */
    GAME_EVENT_UNKNOWN_COMMAND,
    /* #158: inventory/item slice emits generic action + outcome payloads. */
    GAME_EVENT_ITEM_RESULT,
    GAME_EVENT_CORPSE_VIEW,
    GAME_EVENT_BAG_VIEW,
    GAME_EVENT_CRAFT_RESULT,
    GAME_EVENT_EQUIP_RESULT,
    /* #159: combat.c and gprog.c emit generic combat/progression payloads. */
    GAME_EVENT_COMBAT,
    GAME_EVENT_XP_GAIN,
    GAME_EVENT_STAT_CHANGE,
    /* #160: dialogue, roaming NPC, and encounter slice emit actor/dialogue payloads. */
    GAME_EVENT_DIALOGUE,
    GAME_EVENT_ENCOUNTER,
    GAME_EVENT_DIALOGUE_GUARD,
    /* #161: gatmos.c emits environment/observation payloads on world ticks. */
    GAME_EVENT_ENVIRONMENT,
    GAME_EVENT_AMBIENT_NOISE,
    GAME_EVENT_ITEM_PRESENCE,
    GAME_EVENT_OBSERVATION,
    /* #7: gatmos.c post-inspect follow-up menu and reply outcomes. */
    GAME_EVENT_ENV_MENU,
    GAME_EVENT_ENV_RESULT
};

/*
 * Room look payload (#47, #51, #130): ROOM_LOOK arg0=npc room-actor hint;
 * arg1=corpse_present (bit 0) | (weather_kind << 1) | (day_phase << 3)
 * snapshotted at enqueue; arg2/arg3 unused (per-room inspect clues are not on look);
 * room_id and room_item[] hold ground snapshot.
 */

/*
 * Inventory/item payload contract (#158, #129):
 * ITEM_RESULT  arg0=GameEventItemAction arg1=GameEventItemOutcome
 *              arg2=item id or ITEM_NONE arg3=value/capacity/slots when needed
 * CORPSE_VIEW  arg0=non-empty corpse item count arg1=leave-menu choice number
 *              room_id=corpse room; room_item[]=dense corpse slot snapshot
 *              (grendr prints 1..arg0; arg1 is the "leave body" reply index)
 * CRAFT_RESULT arg0=crafted/attempted item id arg1=GameEventCraftOutcome
 * EQUIP_RESULT arg0=item id or ITEM_NONE arg1=GameEventEquipOutcome
 */

/*
 * Dialogue / encounter payload contract (#160):
 * DIALOGUE    arg0=GameEventDialogueActor arg1=GameEventDialoguePhase
 *             arg2=reply choice when PHASE_REPLY; zero otherwise
 *             arg3=actor-specific detail; Herbalist uses HerbalistDialogueScene;
 *             watchman uses WatchmanDialogueScene (#8)
 * ENCOUNTER   arg0=GameEventEncounterKind arg1=action arg2=outcome
 *             arg3=action-specific payload:
 *   OPEN             enemy level for bandits; zero otherwise
 *   GIVE             item id for GIVE replies; zero otherwise
 *             text=item name for GIVE when outcome is OK; unused otherwise
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
    GAME_ITEM_OUTCOME_LEFT_BEHIND,
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
 * Combat/progression payload contract (#159):
 * COMBAT      arg0=GameEventCombatPhase; arg1/arg2/arg3 per phase (combat.c
 *             push_combat_phase; grendr render_combat_event):
 *   START            arg1=player_hp arg2=enemy_hp arg3=enemy_level
 *   ENEMY_DAMAGE     arg1=damage
 *   STATUS           arg1=player_hp arg2=enemy_hp arg3=enemy_level
 *   PLAYER_DAMAGE    arg1=damage
 *   SALVE_HEAL       arg1=player_hp after heal
 *   ENEMY_DEFEATED   arg3=enemy_level (defeat copy)
 *   (other phases leave arg1/arg2/arg3 zero unless noted)
 * XP_GAIN     arg0=amount
 * STAT_CHANGE arg0=level arg1=max_hp arg2=damage_bonus arg3=bag_capacity
 */
enum GameEventCombatPhase {
    GAME_COMBAT_PHASE_NONE = 0,
    GAME_COMBAT_PHASE_START,
    GAME_COMBAT_PHASE_ENEMY_DAMAGE,
    GAME_COMBAT_PHASE_PLAYER_DOWN,
    GAME_COMBAT_PHASE_STATUS,
    GAME_COMBAT_PHASE_PLAYER_DAMAGE,
    GAME_COMBAT_PHASE_BRACED,
    GAME_COMBAT_PHASE_SALVE_NO_BAG,
    GAME_COMBAT_PHASE_SALVE_HEAL,
    GAME_COMBAT_PHASE_SALVE_FULL,
    GAME_COMBAT_PHASE_INVALID_CHOICE,
    GAME_COMBAT_PHASE_ENEMY_DEFEATED,
    GAME_COMBAT_PHASE_MENU
};

/*
 * DIALOGUE_GUARD payload (#160): arg0=GameEventDialogueGuardReason.
 * DIALOGUE / ENCOUNTER arg layout is documented with the event kinds above.
 */
enum GameEventDialogueActor {
    GAME_DIALOGUE_ACTOR_NONE = 0,
    GAME_DIALOGUE_ACTOR_FROG = 1,
    GAME_DIALOGUE_ACTOR_WATCHMAN = 2,
    GAME_DIALOGUE_ACTOR_HERBALIST = 3,
    GAME_DIALOGUE_ACTOR_ARCHIVIST = 4,
    GAME_DIALOGUE_ACTOR_TRAVELER = 5,
    GAME_DIALOGUE_ACTOR_NOBODY = 6,
    /* roster-authored bandits; distinct from BANDIT_AMBUSH dynamic spawn in tests */
    GAME_DIALOGUE_ACTOR_BANDIT = 7,
    GAME_DIALOGUE_ACTOR_BANDIT_BRIDGE = 8,
    GAME_DIALOGUE_ACTOR_BANDIT_CANYON = 9,
    /* dynamic ambush bandit; shares enemy encounter logic */
    GAME_DIALOGUE_ACTOR_BANDIT_AMBUSH = 10,
    /* roaming-friendly actors; reply via npc_roaming_cmd_reply, not genc */
    GAME_DIALOGUE_ACTOR_LOST_ANIMAL = 11,
    GAME_DIALOGUE_ACTOR_PEDDLER = 12
};

enum GameEventDialoguePhase {
    GAME_DIALOGUE_PHASE_NONE = 0,
    GAME_DIALOGUE_PHASE_TALK,
    GAME_DIALOGUE_PHASE_REPLY
};

enum GameEventEncounterKind {
    GAME_ENCOUNTER_NONE = 0,
    GAME_ENCOUNTER_BANDIT,
    GAME_ENCOUNTER_TRAVELER,
    /* dialogue-only OPEN encounters; genc handler rows stay empty */
    GAME_ENCOUNTER_LOST_ANIMAL,
    GAME_ENCOUNTER_PEDDLER
};

enum GameEventEncounterAction {
    GAME_ENCOUNTER_ACTION_NONE = 0,
    GAME_ENCOUNTER_ACTION_OPEN,
    GAME_ENCOUNTER_ACTION_HANDOVER_PROMPT,
    GAME_ENCOUNTER_ACTION_HANDOVER,
    GAME_ENCOUNTER_ACTION_GIVE,
    GAME_ENCOUNTER_ACTION_INTIMIDATE
};

enum GameEventEncounterOutcome {
    GAME_ENCOUNTER_OUTCOME_NONE = 0,
    GAME_ENCOUNTER_OUTCOME_OK,
    GAME_ENCOUNTER_OUTCOME_NOT_CARRYING,
    GAME_ENCOUNTER_OUTCOME_WRONG_CONTEXT,
    GAME_ENCOUNTER_OUTCOME_BAG_EMPTY,
    GAME_ENCOUNTER_OUTCOME_SUCCESS,
    GAME_ENCOUNTER_OUTCOME_FAIL
};

enum GameEventDialogueGuardReason {
    GAME_DIALOGUE_GUARD_NONE = 0,
    GAME_DIALOGUE_GUARD_BANDIT_WAITING_REPLY,
    GAME_DIALOGUE_GUARD_BANDIT_WAITING_HANDOVER_PICK,
    GAME_DIALOGUE_GUARD_BANDIT_BLOCKS_TALK,
    GAME_DIALOGUE_GUARD_LOOT_WAITING_REPLY,
    /* blocks talk while traveler / lost animal / peddler await reply 1/2/3 */
    GAME_DIALOGUE_GUARD_ROAMING_ENCOUNTER_WAITING,
    GAME_DIALOGUE_GUARD_NOBODY_WAITING_REPLY,
    /* non-loot dialogue dismissed by explore verb before the verb runs (#205). */
    GAME_DIALOGUE_GUARD_DIALOGUE_CLOSED,
    /* give/offer with no room NPC handler and no enemy handover active. */
    GAME_DIALOGUE_GUARD_GIVE_NO_TARGET,
    /* room NPC present but refuses the offered item. */
    GAME_DIALOGUE_GUARD_GIVE_REJECTED,
    /* arg1 widens valid reply range; env inspect menus reuse for max_choice (#7). */
    GAME_DIALOGUE_GUARD_PICK_123,
    /* env inspect menu dismissed by explore verb before the verb runs (#7). */
    GAME_DIALOGUE_GUARD_ENV_MENU_CLOSED
};

/*
 * Ambient/inspect payload contract (#161):
 * ENVIRONMENT    arg0=GameEventEnvironmentKind
 * AMBIENT_NOISE  text=room animal noise line
 * ITEM_PRESENCE  arg0=item id; text=item name
 * OBSERVATION    arg0=GameEventObservationOutcome
 * ENV_MENU       arg0=GAME_ENV_* kind arg1=room_id
 * ENV_RESULT     arg0=GAME_ENV_* kind arg1=choice (1-based)
 *                arg2=GameEventEnvResultDetail when needed
 */
enum GameEventEnvResultDetail {
    GAME_ENV_RESULT_DETAIL_NONE = 0,
    GAME_ENV_RESULT_DETAIL_HEALED,
    GAME_ENV_RESULT_DETAIL_HP_FULL,
    GAME_ENV_RESULT_DETAIL_ITEM_SPAWNED,
    GAME_ENV_RESULT_DETAIL_ITEM_FAILED
};
enum GameEventEnvironmentKind {
    GAME_ENV_EVENT_NONE = 0,
    GAME_ENV_EVENT_GUST,
    GAME_ENV_EVENT_RUSTLE,
    GAME_ENV_EVENT_BERRY_DROP,
    GAME_ENV_EVENT_CREAK,
    GAME_ENV_EVENT_WATER,
    GAME_ENV_EVENT_REED_DROP,
    GAME_ENV_EVENT_GRIT,
    /* #51: gatmos_weather_tick transition announcements. */
    GAME_ENV_EVENT_WEATHER_RAIN,
    GAME_ENV_EVENT_WEATHER_FOG,
    GAME_ENV_EVENT_WEATHER_WIND,
    GAME_ENV_EVENT_WEATHER_CLEAR,
    /* #130: gatmos_daynight_tick and night-lost-on-move announcements. */
    GAME_ENV_EVENT_NIGHT_FALL,
    GAME_ENV_EVENT_DAY_BREAK,
    GAME_ENV_EVENT_NIGHT_LOST
};

enum GameEventObservationOutcome {
    GAME_OBS_OUTCOME_NONE = 0,
    GAME_OBS_OUTCOME_NOTHING,
    GAME_OBS_OUTCOME_RUSTLE,
    GAME_OBS_OUTCOME_CREAK,
    GAME_OBS_OUTCOME_WATER,
    GAME_OBS_OUTCOME_GRIT
};

struct GameEvent {
    int kind;   /* GameEventKind */
    int arg0;
    int arg1;
    int arg2;
    int arg3;
    int room_id;
    int room_item[CFG_AREA_ITEM_SLOTS];
    const char *text;
};

struct GameEventQueue {
    struct GameEvent events[CFG_GAME_EVENT_MAX];
    int count;
    int overflowed;
};

typedef struct GameEvent GameEvent;
typedef struct GameEventQueue GameEventQueue;

void game_event_queue_reset(GameEventQueue *out);
GameEvent *game_event_push(GameEventQueue *out, int kind, int arg0, int arg1,
                           int arg2, int arg3, const char *text);

#endif
