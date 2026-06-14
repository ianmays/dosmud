#include "npc.h"
#include "platform.h"
#include "game.h"
#include "gout.h"
#include "world.h"

/*
 * npc.c owns the NPC-facing seam between room identity, dynamic roster
 * placement, and dialogue actors. Higher-level slices still own combat and
 * authored content. Fixed and roaming encounters queue GAME_EVENT_ENCOUNTER /
 * DIALOGUE*; grendr maps copy.
 */

struct NpcRoomInfo {
    int room_id;
    int actor;
    int dialogue_kind;
    int talk_phase;
};

/* Authored enemy spawn rows; stable world rooms like NPC_ROOM_INFO talk hooks. */
struct NpcSeedInfo {
    int actor;
    int dialogue;
    int encounter;
    int room_id;
    int flags;
};

/* Stable content hooks: fixed world rooms, not generated graph membership. */
static const struct NpcRoomInfo NPC_ROOM_INFO[] = {
    /* Frog keeps its custom rendered copy, but event phases now match other NPCs. */
    { WORLD_ROOM_POND, GAME_DIALOGUE_ACTOR_FROG,
        DIALOGUE_NPC_FROG, GAME_DIALOGUE_PHASE_TALK },
    { WORLD_ROOM_TOWER, GAME_DIALOGUE_ACTOR_WATCHMAN,
        DIALOGUE_NPC_WATCHMAN, GAME_DIALOGUE_PHASE_TALK },
    { WORLD_ROOM_ORCHARD, GAME_DIALOGUE_ACTOR_HERBALIST,
        DIALOGUE_NPC_HERBALIST, GAME_DIALOGUE_PHASE_TALK },
    { WORLD_ROOM_CATACOMBS, GAME_DIALOGUE_ACTOR_ARCHIVIST,
        DIALOGUE_NPC_ARCHIVIST, GAME_DIALOGUE_PHASE_TALK }
};

static const struct NpcSeedInfo NPC_FIXED_ENCOUNTERS[] = {
    /* Bandit starts on the road, then roams and respawns through the roster. */
    { GAME_DIALOGUE_ACTOR_BANDIT, DIALOGUE_NONE, GAME_ENCOUNTER_BANDIT,
        WORLD_ROOM_ROAD, NPC_FLAG_ACTIVE | NPC_FLAG_ROAMING | NPC_FLAG_RESPAWNS }
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

static void npc_push_encounter_open(GameEventQueue *out, int kind)
{
    game_event_push(out, GAME_EVENT_ENCOUNTER, kind,
        GAME_ENCOUNTER_ACTION_OPEN, GAME_ENCOUNTER_OUTCOME_NONE, 0, 0);
}

static int npc_encounter_dialogue_kind(const struct NpcState *npc)
{
    if (npc->encounter == GAME_ENCOUNTER_BANDIT) {
        return DIALOGUE_ENEMY;
    }
    return npc->dialogue;
}

static u32 npc_respawn_return_tick(struct GameState *game,
                                   const struct NpcState *npc)
{
    if (npc->encounter == GAME_ENCOUNTER_BANDIT) {
        return game->tick + CFG_BANDIT_RETURN_DELAY_BASE +
            (u32)(plat_rand() % CFG_BANDIT_RETURN_DELAY_SPREAD);
    }
    return 0;
}

static int npc_roaming_can_step(const struct GameState *game,
                                const struct NpcState *npc)
{
    if (npc->encounter == GAME_ENCOUNTER_BANDIT &&
            game->tick < CFG_BANDIT_ROAM_START_TICK) {
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
    int i;

    for (i = 0; i < (int)(sizeof(NPC_ROOM_INFO) / sizeof(NPC_ROOM_INFO[0])); ++i) {
        if (NPC_ROOM_INFO[i].dialogue_kind == dialogue_kind) {
            return NPC_ROOM_INFO[i].actor;
        }
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
    npc_push_encounter_open(out, encounter);
    game_set_mode_dialogue(game, dialogue);
    return slot;
}

/* Immediate encounter teardown; return_tick 0 means no scheduled respawn. */
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
    /* Talk opens dialogue mode and queues one TALK event; reply uses dialogue_cmd_reply. */
    npc_push_dialogue(out, info->actor, info->talk_phase, 0);
    game_set_mode_dialogue(game, info->dialogue_kind);
    return 1;
}

/* Traveler is the first roaming profile; seed sets actor/dialogue/encounter ids. */
void npc_seed_roaming_traveler(struct GameState *game)
{
    npc_spawn(game, GAME_DIALOGUE_ACTOR_TRAVELER, DIALOGUE_TRAVELER,
        GAME_ENCOUNTER_TRAVELER, WORLD_ROOM_RUINS,
        NPC_FLAG_ACTIVE | NPC_FLAG_ROAMING | NPC_FLAG_RESPAWNS);
}

/* Authored enemy profiles seed from NPC_FIXED_ENCOUNTERS on reset. */
void npc_seed_fixed_enemies(struct GameState *game)
{
    int i;

    for (i = 0;
            i < (int)(sizeof(NPC_FIXED_ENCOUNTERS) /
                sizeof(NPC_FIXED_ENCOUNTERS[0]));
            ++i) {
        if (npc_find_by_actor(game, NPC_FIXED_ENCOUNTERS[i].actor) >= 0) {
            continue;
        }
        npc_spawn(game, NPC_FIXED_ENCOUNTERS[i].actor,
            NPC_FIXED_ENCOUNTERS[i].dialogue,
            NPC_FIXED_ENCOUNTERS[i].encounter,
            NPC_FIXED_ENCOUNTERS[i].room_id,
            NPC_FIXED_ENCOUNTERS[i].flags);
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
        npc_push_encounter_open(out, npc->encounter);
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
        npc_push_encounter_open(out, npc->encounter);
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
    npc_deactivate_until(game, npc->actor,
        game->tick + CFG_TRAVELER_RETURN_DELAY_BASE +
        (plat_rand() % CFG_TRAVELER_RETURN_DELAY_SPREAD));
    return 1;
}

/*
 * #160: shared dialogue producers (payload layout in gout.h). Slices queue
 * actor/phase/choice here; grendr maps GAME_EVENT_DIALOGUE* to copy.
 */
void npc_push_dialogue(struct GameEventQueue *out, int actor, int phase, int choice)
{
    game_event_push(out, GAME_EVENT_DIALOGUE, actor, phase, choice, 0, 0);
}

void npc_push_dialogue_guard(struct GameEventQueue *out, int reason)
{
    game_event_push(out, GAME_EVENT_DIALOGUE_GUARD, reason, 0, 0, 0, 0);
}
