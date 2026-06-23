#include "game.h"
#include "dialogue.h"
#include "npc.h"

/*
 * Dialogue dispatch handles fixed room-NPC replies. The npc seam owns room
 * identity and actor lookup so frog and other room NPCs share one path.
 */

int dialogue_cmd_reply(struct GameState *game, int choice, GameEventQueue *out)
{
    /* Enemy and traveler replies stay outside the room-talk authored table. */
    return npc_room_cmd_reply(game, choice, out);
}

int dialogue_cmd_talk(struct GameState *game, GameEventQueue *out)
{
    /* Talk is blocked while the player is already in an enemy or traveler branch. */
    if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_ENEMY) {
        npc_push_dialogue_guard(out, GAME_DIALOGUE_GUARD_BANDIT_BLOCKS_TALK);
        return 1;
    }
    if (game->mode == GAME_MODE_COMBAT) {
        npc_push_dialogue_guard(out, GAME_DIALOGUE_GUARD_BANDIT_BLOCKS_TALK);
        return 1;
    }
    if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_TRAVELER) {
        npc_push_dialogue_guard(out, GAME_DIALOGUE_GUARD_TRAVELER_WAITING);
        return 1;
    }

    if (npc_open_room_dialogue(game, out)) {
        return 1;
    }

    /* Nobody is a one-shot hint; player stays in explore (no dialogue mode). */
    npc_push_dialogue(out, GAME_DIALOGUE_ACTOR_NOBODY,
        GAME_DIALOGUE_PHASE_TALK, 0);
    return 1;
}
