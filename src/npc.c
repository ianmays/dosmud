#include "npc.h"
#include "platform.h"
#include "game.h"
#include "gout.h"
#include "world.h"

/*
 * npc.c owns the NPC-facing seam between room identity, roaming placement, and
 * dialogue actors. Higher-level slices still own combat and authored content.
 */

struct NpcRoomInfo {
    int room_id;
    int actor;
    int dialogue_kind;
    int talk_phase;
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

void npc_seed_roaming_traveler(struct GameState *game)
{
    game->roaming_npc_actor = GAME_DIALOGUE_ACTOR_WANDERER;
    game->roaming_npc_dialogue = DIALOGUE_WANDERER;
    game->roaming_npc_encounter = GAME_ENCOUNTER_WANDERER;
    game->roaming_npc_room = WORLD_ROOM_RUINS;
    game->roaming_npc_need_separation = 0;
    game->roaming_npc_active = 1;
    game->roaming_npc_return_tick = 0;
}

void npc_roaming_update_separation(struct GameState *game)
{
    if (game->player.room_id != game->roaming_npc_room) {
        game->roaming_npc_need_separation = 0;
    }
}

void npc_roaming_step(struct GameState *game)
{
    struct Room *r;
    int dirs[CFG_DIR_MAX];
    int n;
    int i;
    int pick;

    if (game->world.room_count <= 0) {
        return;
    }
    if (game->roaming_npc_room < 0 ||
            game->roaming_npc_room >= game->world.room_count) {
        return;
    }
    r = &game->world.rooms[game->roaming_npc_room];
    n = 0;
    for (i = 0; i < DIR_NONE; ++i) {
        if (r->exits[i] >= 0) {
            dirs[n] = i;
            ++n;
        }
    }
    if (n <= 0) {
        return;
    }
    pick = plat_rand() % n;
    game->roaming_npc_room = r->exits[dirs[pick]];
}

void npc_roaming_begin_encounter(struct GameState *game, GameEventQueue *out)
{
    if (game_is_busy_dialogue(game)) {
        return;
    }
    if (game->roaming_npc_need_separation) {
        return;
    }
    npc_push_encounter_open(out, game->roaming_npc_encounter);
    game_set_mode_dialogue(game, game->roaming_npc_dialogue);
    game->roaming_npc_need_separation = 1;
}

int npc_roaming_cmd_reply(struct GameState *game, int choice, GameEventQueue *out)
{
    if (game->mode != GAME_MODE_DIALOGUE ||
            game->dialogue != game->roaming_npc_dialogue) {
        return 0;
    }
    if (!npc_choice_is_valid(choice)) {
        npc_push_dialogue_guard(out, GAME_DIALOGUE_GUARD_PICK_123);
        return 1;
    }
    npc_push_dialogue(out, game->roaming_npc_actor,
        GAME_DIALOGUE_PHASE_REPLY, choice);
    game_set_mode_explore(game);
    game->roaming_npc_active = 0;
    game->roaming_npc_room = -1;
    game->roaming_npc_return_tick =
        game->tick + CFG_WANDERER_RETURN_DELAY_BASE +
        (plat_rand() % CFG_WANDERER_RETURN_DELAY_SPREAD);
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
