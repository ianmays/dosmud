#include "game.h"
#include "dialogue.h"
#include "grendr.h"
#include "world.h"

/*
 * Dialogue dispatch maps room identity to a small set of fixed NPC branches.
 */

int npc_in_room(int room_id)
{
    /* These room IDs are stable content hooks, not arbitrary checks. */
    if (room_id == WORLD_ROOM_TOWER) return 1;  /* watchman */
    if (room_id == WORLD_ROOM_ORCHARD) return 2;/* herbalist */
    if (room_id == WORLD_ROOM_CATACOMBS) return 3; /* archivist */
    return 0;
}

void frog_dialogue_intro(void)
{
    render_frog_dialogue_intro();
}

void frog_dialogue_branch(int choice)
{
    render_frog_dialogue_branch(choice);
}

int dialogue_cmd_reply(struct GameState *game, int choice)
{
    if (game->mode != GAME_MODE_DIALOGUE) {
        return 0;
    }
    /* Each dialogue kind resolves immediately back to explore after one reply. */
    if (game->dialogue == DIALOGUE_NPC_FROG) {
        if (choice < 1 || choice > 3) {
            render_msg_pick_123();
            return 1;
        }
        frog_dialogue_branch(choice);
        game_set_mode_explore(game);
        return 1;
    }
    if (game->dialogue == DIALOGUE_NPC_WATCHMAN) {
        render_msg_watchman_reply(choice);
        game_set_mode_explore(game);
        return 1;
    }
    if (game->dialogue == DIALOGUE_NPC_HERBALIST) {
        render_msg_herbalist_reply(choice);
        game_set_mode_explore(game);
        return 1;
    }
    if (game->dialogue == DIALOGUE_NPC_ARCHIVIST) {
        render_msg_archivist_reply(choice);
        game_set_mode_explore(game);
        return 1;
    }
    return 0;
}

int dialogue_cmd_talk(struct GameState *game)
{
    /* Talk is blocked while the player is already in an enemy or wanderer branch. */
    if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_ENEMY) {
        render_msg_bandit_blocks_talk();
        return 1;
    }
    if (game->mode == GAME_MODE_COMBAT) {
        render_msg_bandit_blocks_talk();
        return 1;
    }
    if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_WANDERER) {
        render_msg_traveler_waiting();
        return 1;
    }
    if (game->player.room_id == WORLD_ROOM_TOWER) {
        render_msg_watchman_talk();
        game_set_mode_dialogue(game, DIALOGUE_NPC_WATCHMAN);
        return 1;
    }
    if (game->player.room_id == WORLD_ROOM_ORCHARD) {
        render_msg_herbalist_talk();
        game_set_mode_dialogue(game, DIALOGUE_NPC_HERBALIST);
        return 1;
    }
    if (game->player.room_id == WORLD_ROOM_CATACOMBS) {
        render_msg_archivist_talk();
        game_set_mode_dialogue(game, DIALOGUE_NPC_ARCHIVIST);
        return 1;
    }
    if (game->player.room_id != WORLD_ROOM_POND) {
        render_msg_nobody_talk();
        return 1;
    }
    frog_dialogue_intro();
    game_set_mode_dialogue(game, DIALOGUE_NPC_FROG);
    return 1;
}
