#include <string.h>
#include "npc.h"
#include "platform.h"
#include "game.h"
#include "gout.h"
#include "invent.h"
#include "items.h"
#include "txtres.h"
#include "world.h"

/*
 * npc.c owns the NPC-facing seam between room identity, dynamic roster
 * placement, and dialogue actors. Higher-level slices still own combat and
 * authored content. Fixed and roaming encounters queue GAME_EVENT_ENCOUNTER /
 * DIALOGUE*; grendr maps copy.
 */

/*
 * Parallel authored table for fixed room NPCs: talk opens by player room_id;
 * reply resolves by game.dialogue so a mid-branch move does not retarget actor.
 */
struct NpcRoomInfo {
    int room_id;
    int actor;
    int dialogue_kind;
    int open_phase;
    int reply_phase;
};

/* Authored roster placement rows; stable world rooms like NPC_ROOM_INFO talk hooks. */
enum NpcRespawnTrigger {
    NPC_RESPAWN_NEVER = 0,
    NPC_RESPAWN_ON_ENCOUNTER_END,
    NPC_RESPAWN_ON_DIALOGUE_RESOLVE
};

struct NpcProfile {
    int actor;
    int dialogue;
    int encounter;
    /* Authored difficulty band; zero min/max means no combat scaling. */
    int level_min;
    int level_max;
    int spawn_room;
    int flags;
    u32 roam_start_tick;
    u32 respawn_delay_base;
    u32 respawn_delay_spread;
    int respawn_trigger;
};

/*
 * Table order is roster slot order (traveler before bandit) so co-location
 * encounter scans stay deterministic and match the pre-profile seed sequence.
 */
static const struct NpcProfile NPC_PROFILES[] = {
    { GAME_DIALOGUE_ACTOR_TRAVELER, DIALOGUE_TRAVELER, GAME_ENCOUNTER_TRAVELER,
        0, 0,
        WORLD_ROOM_RUINS,
        NPC_FLAG_ACTIVE | NPC_FLAG_ROAMING | NPC_FLAG_RESPAWNS,
        0, 8, 16, NPC_RESPAWN_ON_DIALOGUE_RESOLVE },
    { GAME_DIALOGUE_ACTOR_BANDIT, DIALOGUE_NONE, GAME_ENCOUNTER_BANDIT,
        1, 3,
        WORLD_ROOM_ROAD,
        NPC_FLAG_ACTIVE | NPC_FLAG_ROAMING | NPC_FLAG_RESPAWNS,
        8, 6, 10, NPC_RESPAWN_ON_ENCOUNTER_END },
    { GAME_DIALOGUE_ACTOR_BANDIT_BRIDGE, DIALOGUE_NONE,
        GAME_ENCOUNTER_BANDIT,
        1, 3,
        WORLD_ROOM_BRIDGE,
        NPC_FLAG_ACTIVE | NPC_FLAG_ROAMING | NPC_FLAG_RESPAWNS,
        8, 6, 10, NPC_RESPAWN_ON_ENCOUNTER_END },
    { GAME_DIALOGUE_ACTOR_BANDIT_CANYON, DIALOGUE_NONE,
        GAME_ENCOUNTER_BANDIT,
        1, 3,
        WORLD_ROOM_CANYON,
        NPC_FLAG_ACTIVE | NPC_FLAG_ROAMING | NPC_FLAG_RESPAWNS,
        8, 6, 10, NPC_RESPAWN_ON_ENCOUNTER_END }
};

static const struct NpcProfile *npc_profile_by_actor(int actor)
{
    int i;

    for (i = 0; i < (int)(sizeof(NPC_PROFILES) / sizeof(NPC_PROFILES[0])); ++i) {
        if (NPC_PROFILES[i].actor == actor) {
            return &NPC_PROFILES[i];
        }
    }
    return 0;
}

/* Stable content hooks: fixed world rooms, not generated graph membership. */
static const struct NpcRoomInfo NPC_ROOM_INFO[] = {
    /* Frog keeps its custom rendered copy, but event phases now match other NPCs. */
    { WORLD_ROOM_POND, GAME_DIALOGUE_ACTOR_FROG,
        DIALOGUE_NPC_FROG, GAME_DIALOGUE_PHASE_TALK, GAME_DIALOGUE_PHASE_REPLY },
    { WORLD_ROOM_TOWER, GAME_DIALOGUE_ACTOR_WATCHMAN,
        DIALOGUE_NPC_WATCHMAN, GAME_DIALOGUE_PHASE_TALK,
        GAME_DIALOGUE_PHASE_REPLY },
    { WORLD_ROOM_ORCHARD, GAME_DIALOGUE_ACTOR_HERBALIST,
        DIALOGUE_NPC_HERBALIST, GAME_DIALOGUE_PHASE_TALK,
        GAME_DIALOGUE_PHASE_REPLY },
    { WORLD_ROOM_CATACOMBS, GAME_DIALOGUE_ACTOR_ARCHIVIST,
        DIALOGUE_NPC_ARCHIVIST, GAME_DIALOGUE_PHASE_TALK,
        GAME_DIALOGUE_PHASE_REPLY }
};

/*
 * Roster slots: actor==NONE means vacant; inactive respawn entries keep actor
 * set with NPC_FLAG_ACTIVE cleared. Iteration uses ascending slot index so
 * saves and per-tick roaming walks stay deterministic.
 */
static int npc_slot_is_active(const struct NpcState *npc)
{
    return (npc->flags & NPC_FLAG_ACTIVE) != 0;
}

static int npc_slot_is_roaming(const struct NpcState *npc)
{
    return (npc->flags & NPC_FLAG_ROAMING) != 0;
}

static int npc_slot_needs_separation(const struct NpcState *npc)
{
    return (npc->flags & NPC_FLAG_NEEDS_SEPARATION) != 0;
}

static int npc_slot_respawns(const struct NpcState *npc)
{
    return (npc->flags & NPC_FLAG_RESPAWNS) != 0;
}

/* Non-roaming roster slot with an encounter id; room stays authored, not graph-random. */
static int npc_slot_is_fixed_encounter(const struct NpcState *npc)
{
    return npc->encounter != GAME_ENCOUNTER_NONE &&
        !npc_slot_is_roaming(npc);
}

static struct NpcState *npc_slot(struct GameState *game, int slot)
{
    if (slot < 0 || slot >= CFG_NPC_MAX) {
        return 0;
    }
    return &game->npcs[slot];
}

static const struct NpcState *npc_const_slot(const struct GameState *game, int slot)
{
    if (slot < 0 || slot >= CFG_NPC_MAX) {
        return 0;
    }
    return &game->npcs[slot];
}

/* Only truly vacant slots qualify; deactivated travelers still occupy a slot. */
static int npc_find_free_slot(const struct GameState *game)
{
    int i;

    for (i = 0; i < CFG_NPC_MAX; ++i) {
        if (game->npcs[i].actor == GAME_DIALOGUE_ACTOR_NONE) {
            return i;
        }
    }
    return -1;
}

static struct NpcState *npc_find_dialogue_slot(struct GameState *game, int dialogue_kind)
{
    int i;

    for (i = 0; i < CFG_NPC_MAX; ++i) {
        if (!npc_slot_is_active(&game->npcs[i])) {
            continue;
        }
        if (game->npcs[i].dialogue == dialogue_kind) {
            return &game->npcs[i];
        }
    }
    return 0;
}

static const struct NpcRoomInfo *npc_room_info(int room_id)
{
    int i;

    for (i = 0; i < (int)(sizeof(NPC_ROOM_INFO) / sizeof(NPC_ROOM_INFO[0])); ++i) {
        if (NPC_ROOM_INFO[i].room_id == room_id) {
            return &NPC_ROOM_INFO[i];
        }
    }
    return 0;
}

/*
 * Room talk stays in the parallel table, so dialogue-kind replies resolve
 * through the same authored row that opened the branch.
 */
static const struct NpcRoomInfo *npc_room_dialogue_info(int dialogue_kind)
{
    int i;

    for (i = 0; i < (int)(sizeof(NPC_ROOM_INFO) / sizeof(NPC_ROOM_INFO[0])); ++i) {
        if (NPC_ROOM_INFO[i].dialogue_kind == dialogue_kind) {
            return &NPC_ROOM_INFO[i];
        }
    }
    return 0;
}

/*
 * #76 herbalist vertical slice: npc.c owns story transitions, marsh-root
 * seeding, orchard desc mutation, and scene selection. Reply events keep the
 * pre-choice HerbalistDialogueScene in arg3 so txtres copy matches the menu
 * just closed.
 */
static int npc_room_has_item(const struct GameState *game, int room_id, int item_id)
{
    int slot;

    if (room_id < 0 || room_id >= CFG_ROOM_MAX) {
        return 0;
    }
    for (slot = 0; slot < CFG_AREA_ITEM_SLOTS; ++slot) {
        if (game->room_item[room_id][slot] == item_id) {
            return 1;
        }
    }
    return 0;
}

static int herbalist_dialogue_scene(const struct GameState *game)
{
    /* Keep the persisted seam tiny: ready-to-turn-in is derived from inventory. */
    if (game->herbalist_story == HERBALIST_STORY_COMPLETE) {
        return HERBALIST_SCENE_COMPLETE;
    }
    if (game->herbalist_story == HERBALIST_STORY_REQUESTED) {
        if (game_inv_player_has_item((struct GameState *)game, ITEM_MARSH_ROOT)) {
            return HERBALIST_SCENE_READY;
        }
        return HERBALIST_SCENE_REQUESTED;
    }
    return HERBALIST_SCENE_NOT_STARTED;
}

/* Turn-in swaps orchard desc in-place; incomplete story restores authored baseline. */
static void herbalist_apply_world_hook(struct GameState *game)
{
    if (game->herbalist_story == HERBALIST_STORY_COMPLETE) {
        strncpy(game->world.rooms[WORLD_ROOM_ORCHARD].desc,
            TXT_STORY_ORCHARD_DONE_DESC, CFG_DESC_MAX - 1);
        game->world.rooms[WORLD_ROOM_ORCHARD].desc[CFG_DESC_MAX - 1] = '\0';
        return;
    }
    strncpy(game->world.rooms[WORLD_ROOM_ORCHARD].desc,
        g_room_descs[WORLD_ROOM_ORCHARD], CFG_DESC_MAX - 1);
    game->world.rooms[WORLD_ROOM_ORCHARD].desc[CFG_DESC_MAX - 1] = '\0';
}

static void herbalist_seed_marsh_root(struct GameState *game)
{
    int slot;

    /* Keep one recoverable root in play when the requested story beat is active. */
    if (game_inv_player_has_item(game, ITEM_MARSH_ROOT) ||
            npc_room_has_item(game, WORLD_ROOM_MARSH, ITEM_MARSH_ROOT)) {
        game->marsh_root_spawned = 1;
        return;
    }
    game->marsh_root_spawned = 0;
    for (slot = 0; slot < CFG_AREA_ITEM_SLOTS; ++slot) {
        if (game->room_item[WORLD_ROOM_MARSH][slot] == ITEM_NONE) {
            game->room_item[WORLD_ROOM_MARSH][slot] = ITEM_MARSH_ROOT;
            game->marsh_root_spawned = 1;
            return;
        }
    }
}

static int herbalist_open_dialogue(struct GameState *game, GameEventQueue *out)
{
    int scene;

    if (game->herbalist_story == HERBALIST_STORY_REQUESTED) {
        herbalist_seed_marsh_root(game);
    }
    herbalist_apply_world_hook(game);
    scene = herbalist_dialogue_scene(game);
    npc_push_dialogue_detail(out, GAME_DIALOGUE_ACTOR_HERBALIST,
        GAME_DIALOGUE_PHASE_TALK, 0, scene);
    game_set_mode_dialogue(game, DIALOGUE_NPC_HERBALIST);
    return 1;
}

static int herbalist_reply_not_started(struct GameState *game, int choice,
                                       GameEventQueue *out)
{
    if (choice == 1) {
        game->herbalist_story = HERBALIST_STORY_REQUESTED;
        herbalist_seed_marsh_root(game);
    }
    npc_push_dialogue_detail(out, GAME_DIALOGUE_ACTOR_HERBALIST,
        GAME_DIALOGUE_PHASE_REPLY, choice, HERBALIST_SCENE_NOT_STARTED);
    game_set_mode_explore(game);
    return 1;
}

static int herbalist_reply_requested(struct GameState *game, int choice,
                                     GameEventQueue *out)
{
    herbalist_seed_marsh_root(game);
    npc_push_dialogue_detail(out, GAME_DIALOGUE_ACTOR_HERBALIST,
        GAME_DIALOGUE_PHASE_REPLY, choice, HERBALIST_SCENE_REQUESTED);
    game_set_mode_explore(game);
    return 1;
}

static int herbalist_reply_ready(struct GameState *game, int choice,
                                 GameEventQueue *out)
{
    if (choice == 1) {
        game_inv_bag_remove_item(game, ITEM_MARSH_ROOT);
        game->herbalist_story = HERBALIST_STORY_COMPLETE;
        herbalist_apply_world_hook(game);
    }
    npc_push_dialogue_detail(out, GAME_DIALOGUE_ACTOR_HERBALIST,
        GAME_DIALOGUE_PHASE_REPLY, choice, HERBALIST_SCENE_READY);
    game_set_mode_explore(game);
    return 1;
}

static int herbalist_reply_complete(struct GameState *game, int choice,
                                    GameEventQueue *out)
{
    herbalist_apply_world_hook(game);
    npc_push_dialogue_detail(out, GAME_DIALOGUE_ACTOR_HERBALIST,
        GAME_DIALOGUE_PHASE_REPLY, choice, HERBALIST_SCENE_COMPLETE);
    game_set_mode_explore(game);
    return 1;
}

static int herbalist_reply(struct GameState *game, int choice,
                           GameEventQueue *out)
{
    int scene;

    scene = herbalist_dialogue_scene(game);
    if (scene == HERBALIST_SCENE_COMPLETE) {
        return herbalist_reply_complete(game, choice, out);
    }
    if (scene == HERBALIST_SCENE_READY) {
        return herbalist_reply_ready(game, choice, out);
    }
    if (scene == HERBALIST_SCENE_REQUESTED) {
        return herbalist_reply_requested(game, choice, out);
    }
    return herbalist_reply_not_started(game, choice, out);
}

static const struct NpcProfile *npc_bandit_profile(void)
{
    return npc_profile_by_actor(GAME_DIALOGUE_ACTOR_BANDIT);
}

static int npc_roll_profile_level(const struct GameState *game,
                                  const struct NpcProfile *profile,
                                  u32 salt)
{
    u32 span;

    /* Seed, actor, and room/tick salt keep level rolls stable across save/load. */
    if (profile == 0 || profile->level_max <= 0) {
        return 0;
    }
    if (profile->level_max <= profile->level_min) {
        return profile->level_min;
    }
    span = (u32)(profile->level_max - profile->level_min + 1);
    return profile->level_min + (int)((game->seed + (u32)profile->actor +
        salt) % span);
}

static int npc_default_level_for_encounter(const struct GameState *game,
                                           int actor, int encounter, u32 salt)
{
    const struct NpcProfile *profile;

    profile = npc_profile_by_actor(actor);
    if (profile != 0) {
        return npc_roll_profile_level(game, profile, salt);
    }
    if (encounter == GAME_ENCOUNTER_BANDIT) {
        /* Actors without a profile row (e.g. BANDIT_AMBUSH) reuse road bandit scaling. */
        profile = npc_bandit_profile();
        if (profile != 0) {
            return npc_roll_profile_level(game, profile, salt);
        }
        return 1;
    }
    return 0;
}

static void npc_push_encounter_open(GameEventQueue *out, int kind, int level)
{
    /* OPEN arg3 carries bandit level for grendr; zero for non-scaling encounters. */
    game_event_push(out, GAME_EVENT_ENCOUNTER, kind,
        GAME_ENCOUNTER_ACTION_OPEN, GAME_ENCOUNTER_OUTCOME_NONE, level, 0);
}

/*
 * Bandit seed rows keep dialogue NONE; genc and combat still gate on
 * DIALOGUE_ENEMY once a roaming encounter opens in the player's room.
 */
static int npc_encounter_dialogue_kind(const struct NpcState *npc)
{
    if (npc->encounter == GAME_ENCOUNTER_BANDIT) {
        return DIALOGUE_ENEMY;
    }
    return npc->dialogue;
}

/* Respawn delay from the authored profile row when encounter teardown schedules it. */
static u32 npc_respawn_return_tick(struct GameState *game,
                                   const struct NpcState *npc)
{
    const struct NpcProfile *profile;

    profile = npc_profile_by_actor(npc->actor);
    if (profile == 0 ||
            profile->respawn_trigger != NPC_RESPAWN_ON_ENCOUNTER_END) {
        return 0;
    }
    return game->tick + profile->respawn_delay_base +
        (u32)(plat_rand() % profile->respawn_delay_spread);
}

/* roam_start_tick on the profile keeps early road encounter beats seed-stable. */
static int npc_roaming_can_step(const struct GameState *game,
                                const struct NpcState *npc)
{
    const struct NpcProfile *profile;

    profile = npc_profile_by_actor(npc->actor);
    if (profile != 0 && game->tick < profile->roam_start_tick) {
        return 0;
    }
    return 1;
}

int npc_room_actor(int room_id)
{
    const struct NpcRoomInfo *info;

    info = npc_room_info(room_id);
    if (info == 0) {
        return GAME_DIALOGUE_ACTOR_NONE;
    }
    return info->actor;
}

/* Enemy replies stay in genc.c; traveler and room NPCs are all NPC-owned. */
int npc_dialogue_actor(int dialogue_kind)
{
    const struct NpcRoomInfo *info;

    info = npc_room_dialogue_info(dialogue_kind);
    if (info != 0) {
        return info->actor;
    }
    return GAME_DIALOGUE_ACTOR_NONE;
}

int npc_choice_is_valid(int choice)
{
    return choice >= 1 && choice <= 3;
}

void npc_clear_all(struct GameState *game)
{
    int i;

    for (i = 0; i < CFG_NPC_MAX; ++i) {
        game->npcs[i].actor = GAME_DIALOGUE_ACTOR_NONE;
        game->npcs[i].dialogue = DIALOGUE_NONE;
        game->npcs[i].encounter = GAME_ENCOUNTER_NONE;
        game->npcs[i].level = 0;
        game->npcs[i].room_id = -1;
        game->npcs[i].flags = 0;
        game->npcs[i].return_tick = 0;
    }
}

int npc_find_by_actor(const struct GameState *game, int actor)
{
    int i;

    for (i = 0; i < CFG_NPC_MAX; ++i) {
        if (game->npcs[i].actor == actor) {
            return i;
        }
    }
    return -1;
}

/* Active slot for a dialogue kind; inactive respawn profiles are skipped. */
int npc_find_by_dialogue(const struct GameState *game, int dialogue)
{
    int i;

    for (i = 0; i < CFG_NPC_MAX; ++i) {
        if (!npc_slot_is_active(&game->npcs[i])) {
            continue;
        }
        if (game->npcs[i].dialogue == dialogue) {
            return i;
        }
    }
    return -1;
}

int npc_find_in_room(const struct GameState *game, int room_id)
{
    int i;

    for (i = 0; i < CFG_NPC_MAX; ++i) {
        if (!npc_slot_is_active(&game->npcs[i])) {
            continue;
        }
        if (game->npcs[i].room_id == room_id) {
            return i;
        }
    }
    return -1;
}

int npc_spawn(struct GameState *game, int actor, int dialogue, int encounter,
              int room_id, int flags)
{
    int slot;
    struct NpcState *npc;

    slot = npc_find_by_actor(game, actor);
    if (slot < 0) {
        slot = npc_find_free_slot(game);
        if (slot < 0) {
            return -1;
        }
    }
    npc = npc_slot(game, slot);
    npc->actor = actor;
    npc->dialogue = dialogue;
    npc->encounter = encounter;
    npc->level = npc_default_level_for_encounter(game, actor, encounter,
        (u32)(room_id >= 0 ? room_id : 0));
    npc->room_id = room_id;
    npc->flags = flags;
    npc->return_tick = 0;
    return slot;
}

int npc_place(struct GameState *game, int actor, int room_id, int flags)
{
    int slot;
    struct NpcState *npc;

    slot = npc_find_by_actor(game, actor);
    if (slot < 0) {
        return -1;
    }
    npc = npc_slot(game, slot);
    npc->room_id = room_id;
    npc->flags = flags;
    return slot;
}

int npc_move(struct GameState *game, int actor, int room_id)
{
    int slot;
    struct NpcState *npc;

    slot = npc_find_by_actor(game, actor);
    if (slot < 0) {
        return -1;
    }
    npc = npc_slot(game, slot);
    npc->room_id = room_id;
    return slot;
}

int npc_is_present(const struct GameState *game, int actor, int room_id)
{
    int slot;
    const struct NpcState *npc;

    slot = npc_find_by_actor(game, actor);
    if (slot < 0) {
        return 0;
    }
    npc = npc_const_slot(game, slot);
    return npc_slot_is_active(npc) && npc->room_id == room_id;
}

/* Clears presence but keeps the roster profile for respawn or fixture reuse. */
int npc_deactivate_until(struct GameState *game, int actor, u32 return_tick)
{
    int slot;
    struct NpcState *npc;

    slot = npc_find_by_actor(game, actor);
    if (slot < 0) {
        return -1;
    }
    npc = npc_slot(game, slot);
    if (npc->encounter == GAME_ENCOUNTER_BANDIT) {
        /* Restore idle roaming profile; dialogue is set again on next co-location open. */
        npc->dialogue = DIALOGUE_NONE;
    }
    npc->flags &= ~(NPC_FLAG_ACTIVE | NPC_FLAG_NEEDS_SEPARATION |
        NPC_FLAG_HANDOVER_PICK);
    npc->room_id = -1;
    npc->return_tick = return_tick;
    return slot;
}

int npc_begin_encounter(struct GameState *game, int actor, int dialogue,
                        int encounter, int room_id, int flags,
                        struct GameEventQueue *out)
{
    int slot;

    /* Dynamic encounter owners claim a roster slot before dialogue mode opens. */
    if (game_is_busy_dialogue(game)) {
        return -1;
    }
    slot = npc_spawn(game, actor, dialogue, encounter, room_id,
        flags | NPC_FLAG_ACTIVE);
    if (slot < 0) {
        return -1;
    }
    npc_push_encounter_open(out, encounter, game->npcs[slot].level);
    game_set_mode_dialogue(game, dialogue);
    return slot;
}

/*
 * Immediate encounter teardown. Respawning roster profiles schedule
 * return_tick here; 0 keeps the slot inactive with no due reactivation.
 */
int npc_end_encounter(struct GameState *game, int actor)
{
    int slot;
    struct NpcState *npc;
    u32 return_tick;

    slot = npc_find_by_actor(game, actor);
    if (slot < 0) {
        return -1;
    }
    npc = npc_slot(game, slot);
    return_tick = 0;
    if (npc_slot_respawns(npc)) {
        return_tick = npc_respawn_return_tick(game, npc);
    }
    return npc_deactivate_until(game, actor, return_tick);
}

int npc_open_room_dialogue(struct GameState *game, struct GameEventQueue *out)
{
    const struct NpcRoomInfo *info;

    info = npc_room_info(game->player.room_id);
    if (info == 0) {
        return 0;
    }
    /* Multi-scene herbalist bypasses generic npc_push_dialogue (needs arg3 scene). */
    if (info->dialogue_kind == DIALOGUE_NPC_HERBALIST) {
        return herbalist_open_dialogue(game, out);
    }
    /* Talk opens dialogue mode and queues one TALK event; reply uses npc_room_cmd_reply. */
    npc_push_dialogue(out, info->actor, info->open_phase, 0);
    game_set_mode_dialogue(game, info->dialogue_kind);
    return 1;
}

/*
 * Room-talk reply path: keyed on game->dialogue, not player room. Non-room
 * kinds (traveler, enemy) return 0 so genc and roaming slices keep ownership.
 */
int npc_room_cmd_reply(struct GameState *game, int choice, GameEventQueue *out)
{
    const struct NpcRoomInfo *info;

    if (game->mode != GAME_MODE_DIALOGUE) {
        return 0;
    }
    info = npc_room_dialogue_info(game->dialogue);
    if (info == 0) {
        return 0;
    }
    if (!npc_choice_is_valid(choice)) {
        npc_push_dialogue_guard(out, GAME_DIALOGUE_GUARD_PICK_123);
        return 1;
    }
    if (info->dialogue_kind == DIALOGUE_NPC_HERBALIST) {
        return herbalist_reply(game, choice, out);
    }
    npc_push_dialogue(out, info->actor, info->reply_phase, choice);
    game_set_mode_explore(game);
    return 1;
}

/* Seed authored roster profiles after npc_clear_all (reset_mutable_state). */
void npc_seed_profiles(struct GameState *game)
{
    int i;
    const struct NpcProfile *profile;

    for (i = 0; i < (int)(sizeof(NPC_PROFILES) / sizeof(NPC_PROFILES[0])); ++i) {
        profile = &NPC_PROFILES[i];
        if (npc_find_by_actor(game, profile->actor) >= 0) {
            continue;
        }
        npc_spawn(game, profile->actor, profile->dialogue, profile->encounter,
            profile->spawn_room, profile->flags);
    }
}

/* Reactivation after return_tick picks a random room in the generated graph. */
void npc_roaming_activate_due(struct GameState *game)
{
    int i;

    if (game->world.room_count <= 0) {
        return;
    }
    for (i = 0; i < CFG_NPC_MAX; ++i) {
        if (npc_slot_is_active(&game->npcs[i])) {
            continue;
        }
        if (!npc_slot_is_roaming(&game->npcs[i]) ||
                !npc_slot_respawns(&game->npcs[i])) {
            continue;
        }
        if (game->tick < game->npcs[i].return_tick) {
            continue;
        }
        game->npcs[i].flags |= NPC_FLAG_ACTIVE;
        if (game->npcs[i].encounter == GAME_ENCOUNTER_BANDIT) {
            game->npcs[i].dialogue = DIALOGUE_NONE;
        }
        game->npcs[i].room_id = plat_rand() % game->world.room_count;
        /* Re-roll level from return_tick + room so respawns stay seed-stable. */
        game->npcs[i].level = npc_default_level_for_encounter(game,
            game->npcs[i].actor, game->npcs[i].encounter,
            game->npcs[i].return_tick + (u32)game->npcs[i].room_id);
    }
}

void npc_roaming_update_separation(struct GameState *game)
{
    int i;

    /* Once the player leaves the roaming room, the re-encounter lock can clear. */
    for (i = 0; i < CFG_NPC_MAX; ++i) {
        if (!npc_slot_is_active(&game->npcs[i]) ||
                !npc_slot_is_roaming(&game->npcs[i])) {
            continue;
        }
        if (game->player.room_id != game->npcs[i].room_id) {
            game->npcs[i].flags &= ~NPC_FLAG_NEEDS_SEPARATION;
        }
    }
}

void npc_roaming_step(struct GameState *game)
{
    struct Room *r;
    int dirs[CFG_DIR_MAX];
    int n;
    int i;
    int pick;
    int slot;

    /* Movement is bounded by the generated graph; invalid room state is a no-op. */
    if (game->world.room_count <= 0) {
        return;
    }
    for (slot = 0; slot < CFG_NPC_MAX; ++slot) {
        if (!npc_slot_is_active(&game->npcs[slot]) ||
                !npc_slot_is_roaming(&game->npcs[slot])) {
            continue;
        }
        if (!npc_roaming_can_step(game, &game->npcs[slot])) {
            continue;
        }
        if (game->npcs[slot].room_id < 0 ||
                game->npcs[slot].room_id >= game->world.room_count) {
            continue;
        }
        r = &game->world.rooms[game->npcs[slot].room_id];
        n = 0;
        for (i = 0; i < DIR_NONE; ++i) {
            if (r->exits[i] >= 0) {
                dirs[n] = i;
                ++n;
            }
        }
        if (n <= 0) {
            continue;
        }
        pick = plat_rand() % n;
        game->npcs[slot].room_id = r->exits[dirs[pick]];
    }
}

/*
 * Returns 1 when a fixed encounter opened. Lowest matching slot wins; slot
 * and room_id stay put (no separation flag; genc still owns reply/give).
 */
int npc_fixed_begin_encounter_in_room(struct GameState *game, int room_id,
                                      GameEventQueue *out)
{
    int slot;
    struct NpcState *npc;

    if (game_is_busy_dialogue(game)) {
        return 0;
    }
    for (slot = 0; slot < CFG_NPC_MAX; ++slot) {
        npc = npc_slot(game, slot);
        if (!npc_slot_is_active(npc) ||
                !npc_slot_is_fixed_encounter(npc) ||
                npc->room_id != room_id) {
            continue;
        }
        /* Fixed encounters keep the authored slot; only mode/event state changes. */
        npc->dialogue = DIALOGUE_ENEMY;
        npc_push_encounter_open(out, npc->encounter, npc->level);
        game_set_mode_dialogue(game, npc->dialogue);
        return 1;
    }
    return 0;
}

/*
 * Returns 1 when an encounter opened. Lowest matching slot wins when several
 * roaming NPCs share room_id; separation prevents immediate retrigger.
 */
int npc_roaming_begin_encounter_in_room(struct GameState *game, int room_id,
                                        GameEventQueue *out)
{
    int slot;
    struct NpcState *npc;

    if (game_is_busy_dialogue(game)) {
        return 0;
    }
    for (slot = 0; slot < CFG_NPC_MAX; ++slot) {
        npc = npc_slot(game, slot);
        if (!npc_slot_is_active(npc) ||
                !npc_slot_is_roaming(npc) ||
                npc->room_id != room_id ||
                npc_slot_needs_separation(npc)) {
            continue;
        }
        npc_push_encounter_open(out, npc->encounter, npc->level);
        npc->dialogue = npc_encounter_dialogue_kind(npc);
        game_set_mode_dialogue(game, npc->dialogue);
        npc->flags |= NPC_FLAG_NEEDS_SEPARATION;
        return 1;
    }
    return 0;
}

void npc_roaming_begin_encounter(struct GameState *game, GameEventQueue *out)
{
    (void)npc_roaming_begin_encounter_in_room(game, game->player.room_id, out);
}

int npc_roaming_cmd_reply(struct GameState *game, int choice, GameEventQueue *out)
{
    struct NpcState *npc;

    /* Return 0 when game.c should try another reply slice or emit a guard. */
    if (game->mode != GAME_MODE_DIALOGUE ||
            game->dialogue != DIALOGUE_TRAVELER) {
        return 0;
    }
    npc = npc_find_dialogue_slot(game, game->dialogue);
    if (npc == 0) {
        return 0;
    }
    if (!npc_choice_is_valid(choice)) {
        npc_push_dialogue_guard(out, GAME_DIALOGUE_GUARD_PICK_123);
        return 1;
    }
    npc_push_dialogue(out, npc->actor, GAME_DIALOGUE_PHASE_REPLY, choice);
    game_set_mode_explore(game);
    /* Return timing is randomized only after the player resolves the branch. */
    {
        const struct NpcProfile *profile;

        profile = npc_profile_by_actor(npc->actor);
        if (profile != 0 &&
                profile->respawn_trigger == NPC_RESPAWN_ON_DIALOGUE_RESOLVE) {
            npc_deactivate_until(game, npc->actor,
                game->tick + profile->respawn_delay_base +
                (u32)(plat_rand() % profile->respawn_delay_spread));
        }
    }
    return 1;
}

/*
 * #160: shared dialogue producers (payload layout in gout.h). Slices queue
 * actor/phase/choice here; detail becomes arg3 (e.g. HerbalistDialogueScene).
 * grendr maps GAME_EVENT_DIALOGUE* to copy.
 */
void npc_push_dialogue_detail(struct GameEventQueue *out, int actor, int phase,
                              int choice, int detail)
{
    game_event_push(out, GAME_EVENT_DIALOGUE, actor, phase, choice, detail, 0);
}

void npc_push_dialogue(struct GameEventQueue *out, int actor, int phase, int choice)
{
    npc_push_dialogue_detail(out, actor, phase, choice, 0);
}

void npc_push_dialogue_guard(struct GameEventQueue *out, int reason)
{
    game_event_push(out, GAME_EVENT_DIALOGUE_GUARD, reason, 0, 0, 0, 0);
}
