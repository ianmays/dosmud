#include "game.h"
#include "dialogue.h"
#include "npc.h"

/*
 * Dialogue dispatch handles fixed room-NPC replies. The npc seam owns room
 * identity and actor lookup so frog and other room NPCs share one path.
 */

int dialogue_cmd_reply(struct GameState *game, int choice, GameEventQueue *out)
{
    int actor;

    if (game->mode != GAME_MODE_DIALOGUE) {
        return 0;
    }

    actor = npc_dialogue_actor(game->dialogue);
    /* Enemy replies stay in genc.c; traveler replies now route through npc.c. */
    if (actor == GAME_DIALOGUE_ACTOR_NONE) {
        return 0;
    }
    if (!npc_choice_is_valid(choice)) {
        npc_push_dialogue_guard(out, GAME_DIALOGUE_GUARD_PICK_123);
        return 1;
    }
    /* Each fixed room NPC resolves one reply and returns to explore. */
    npc_push_dialogue(out, actor, GAME_DIALOGUE_PHASE_REPLY, choice);
    game_set_mode_explore(game);
    return 1;
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
    if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_WANDERER) {
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
