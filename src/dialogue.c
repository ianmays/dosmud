#include "game.h"
#include "dialogue.h"
#include "npc.h"

/*
 * Dialogue command dispatch: talk guards, nobody hint, and room-talk open.
 * Roaming-friendly replies (traveler, lost animal, peddler) stay in npc.c;
 * room-talk replies delegate to npc_room_cmd_reply (authored table keyed by
 * game.dialogue, not player room).
 */

int dialogue_cmd_reply(struct GameState *game, int choice, GameEventQueue *out)
{
    /* Enemy and roaming-friendly replies stay outside the room-talk table. */
    return npc_room_cmd_reply(game, choice, out);
}

int dialogue_cmd_talk(struct GameState *game, GameEventQueue *out)
{
    /* Talk is blocked while enemy combat dialogue or a roaming branch is open. */
    if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_ENEMY) {
        npc_push_dialogue_guard(out, GAME_DIALOGUE_GUARD_BANDIT_BLOCKS_TALK);
        return 1;
    }
    if (game->mode == GAME_MODE_COMBAT) {
        npc_push_dialogue_guard(out, GAME_DIALOGUE_GUARD_BANDIT_BLOCKS_TALK);
        return 1;
    }
    if (game->mode == GAME_MODE_DIALOGUE &&
            npc_is_roaming_friendly_dialogue(game->dialogue)) {
        (void)npc_replay_active_prompt(game, out);
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
