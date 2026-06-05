#include "game.h"
#include "dialogue.h"
#include "gout.h"
#include "world.h"

/*
 * Dialogue dispatch maps room identity to a small set of fixed NPC branches.
 * #160: queues GAME_EVENT_DIALOGUE / DIALOGUE_GUARD; grendr maps to text.
 */

static void push_dialogue(struct GameOutput *out, int actor, int phase,
                          int choice)
{
    game_event_push(out, GAME_EVENT_DIALOGUE, actor, phase, choice, 0, 0);
}

static void push_dialogue_guard(struct GameOutput *out, int reason)
{
    game_event_push(out, GAME_EVENT_DIALOGUE_GUARD, reason, 0, 0, 0, 0);
}

int npc_in_room(int room_id)
{
    /* These room IDs are stable content hooks, not arbitrary checks. */
    if (room_id == WORLD_ROOM_TOWER) return 1;  /* watchman */
    if (room_id == WORLD_ROOM_ORCHARD) return 2;/* herbalist */
    if (room_id == WORLD_ROOM_CATACOMBS) return 3; /* archivist */
    return 0;
}

void frog_dialogue_intro(struct GameOutput *out)
{
    push_dialogue(out, GAME_DIALOGUE_ACTOR_FROG, GAME_DIALOGUE_PHASE_INTRO, 0);
}

void frog_dialogue_branch(int choice, struct GameOutput *out)
{
    push_dialogue(out, GAME_DIALOGUE_ACTOR_FROG, GAME_DIALOGUE_PHASE_BRANCH,
        choice);
}

int dialogue_cmd_reply(struct GameState *game, int choice, struct GameOutput *out)
{
    if (game->mode != GAME_MODE_DIALOGUE) {
        return 0;
    }
    /* Each dialogue kind resolves immediately back to explore after one reply. */
    if (game->dialogue == DIALOGUE_NPC_FROG) {
        if (choice < 1 || choice > 3) {
            push_dialogue_guard(out, GAME_DIALOGUE_GUARD_PICK_123);
            return 1;
        }
        frog_dialogue_branch(choice, out);
        game_set_mode_explore(game);
        return 1;
    }
    if (game->dialogue == DIALOGUE_NPC_WATCHMAN) {
        push_dialogue(out, GAME_DIALOGUE_ACTOR_WATCHMAN,
            GAME_DIALOGUE_PHASE_REPLY, choice);
        game_set_mode_explore(game);
        return 1;
    }
    if (game->dialogue == DIALOGUE_NPC_HERBALIST) {
        push_dialogue(out, GAME_DIALOGUE_ACTOR_HERBALIST,
            GAME_DIALOGUE_PHASE_REPLY, choice);
        game_set_mode_explore(game);
        return 1;
    }
    if (game->dialogue == DIALOGUE_NPC_ARCHIVIST) {
        push_dialogue(out, GAME_DIALOGUE_ACTOR_ARCHIVIST,
            GAME_DIALOGUE_PHASE_REPLY, choice);
        game_set_mode_explore(game);
        return 1;
    }
    return 0;
}

int dialogue_cmd_talk(struct GameState *game, struct GameOutput *out)
{
    /* Talk is blocked while the player is already in an enemy or wanderer branch. */
    if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_ENEMY) {
        push_dialogue_guard(out, GAME_DIALOGUE_GUARD_BANDIT_BLOCKS_TALK);
        return 1;
    }
    if (game->mode == GAME_MODE_COMBAT) {
        push_dialogue_guard(out, GAME_DIALOGUE_GUARD_BANDIT_BLOCKS_TALK);
        return 1;
    }
    if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_WANDERER) {
        push_dialogue_guard(out, GAME_DIALOGUE_GUARD_TRAVELER_WAITING);
        return 1;
    }
    if (game->player.room_id == WORLD_ROOM_TOWER) {
        push_dialogue(out, GAME_DIALOGUE_ACTOR_WATCHMAN,
            GAME_DIALOGUE_PHASE_TALK, 0);
        game_set_mode_dialogue(game, DIALOGUE_NPC_WATCHMAN);
        return 1;
    }
    if (game->player.room_id == WORLD_ROOM_ORCHARD) {
        push_dialogue(out, GAME_DIALOGUE_ACTOR_HERBALIST,
            GAME_DIALOGUE_PHASE_TALK, 0);
        game_set_mode_dialogue(game, DIALOGUE_NPC_HERBALIST);
        return 1;
    }
    if (game->player.room_id == WORLD_ROOM_CATACOMBS) {
        push_dialogue(out, GAME_DIALOGUE_ACTOR_ARCHIVIST,
            GAME_DIALOGUE_PHASE_TALK, 0);
        game_set_mode_dialogue(game, DIALOGUE_NPC_ARCHIVIST);
        return 1;
    }
    if (game->player.room_id != WORLD_ROOM_POND) {
        push_dialogue(out, GAME_DIALOGUE_ACTOR_NOBODY,
            GAME_DIALOGUE_PHASE_TALK, 0);
        return 1;
    }
    frog_dialogue_intro(out);
    game_set_mode_dialogue(game, DIALOGUE_NPC_FROG);
    return 1;
}
