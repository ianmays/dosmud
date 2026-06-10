#include "npc.h"
#include "game.h"
#include "gout.h"
#include "world.h"

/*
 * npc.c owns the fixed seam between room identity and NPC-facing dialogue
 * actors. Higher-level slices still own encounter timing, combat, and content.
 */

struct NpcRoomInfo {
    int room_id;
    int actor;
    int dialogue_kind;
    int talk_phase;
};

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

int npc_room_actor(int room_id)
{
    const struct NpcRoomInfo *info;

    info = npc_room_info(room_id);
    if (info == 0) {
        return GAME_DIALOGUE_ACTOR_NONE;
    }
    return info->actor;
}

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
    /* Fixed room NPCs all open through this seam; only the phase varies. */
    npc_push_dialogue(out, info->actor, info->talk_phase, 0);
    game_set_mode_dialogue(game, info->dialogue_kind);
    return 1;
}

void npc_push_dialogue(struct GameEventQueue *out, int actor, int phase, int choice)
{
    game_event_push(out, GAME_EVENT_DIALOGUE, actor, phase, choice, 0, 0);
}

void npc_push_dialogue_guard(struct GameEventQueue *out, int reason)
{
    game_event_push(out, GAME_EVENT_DIALOGUE_GUARD, reason, 0, 0, 0, 0);
}
