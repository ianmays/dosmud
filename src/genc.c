#include "genc.h"
#include "combat.h"
#include "game.h"
#include "gout.h"
#include "invent.h"
#include "items.h"

/*
 * Enemy encounter handling stays separate from combat so the dialogue
 * branch can gate handover and intimidation.
 */

void enemy_begin_encounter(struct GameState *game, struct GameOutput *out)
{
    if (game_is_busy_dialogue(game)) {
        return;
    }
    gout_push(out, GAME_OUT_BANDIT_ENCOUNTER_OPEN, 0, 0, 0, 0, 0);
    game_set_mode_dialogue(game, DIALOGUE_ENEMY);
}

int genc_cmd_give(struct GameState *game, int item_arg, struct GameOutput *out)
{
    if (game->mode == GAME_MODE_DIALOGUE &&
            game->dialogue == DIALOGUE_ENEMY &&
            game->enemy_handover_pick == 1) {
        if (!game_inv_player_has_item(game, item_arg)) {
            gout_push(out, GAME_OUT_MSG_BANDIT_GIVE_NOT_CARRYING, 0, 0, 0, 0, 0);
            return 1;
        }
        gout_push(out, GAME_OUT_MSG_HAND_OVER_ITEM, 0, 0, 0, 0, item_name(item_arg));
        if (game->weapon_equipped == item_arg) {
            game->weapon_equipped = ITEM_NONE;
        } else {
            game_inv_bag_remove_item(game, item_arg);
        }
        game_set_mode_explore(game);
        return 1;
    }
    gout_push(out, GAME_OUT_MSG_GIVE_WRONG_CONTEXT, 0, 0, 0, 0, 0);
    return 1;
}

int genc_cmd_reply(struct GameState *game, int choice, struct GameOutput *out)
{
    /* Reply choice decides whether the enemy opens combat, asks for tribute, or reacts to intimidation. */
    if (choice == 1) {
        combat_start(game, out);
        return 1;
    }
    if (choice == 2) {
        if (game->bag_count <= 0 && game->weapon_equipped == ITEM_NONE) {
            gout_push(out, GAME_OUT_MSG_BAG_EMPTY_BANDIT, 0, 0, 0, 0, 0);
            combat_start(game, out);
            return 1;
        }
        game->enemy_handover_pick = 1;
        gout_push(out, GAME_OUT_BANDIT_HANDOVER_PICK_PROMPT, 0, 0, 0, 0, 0);
        return 1;
    }
    if (choice == 3) {
        game->enemy_handover_pick = 0;
        if (game_roll_percent(game) < CFG_BANDIT_INTIMIDATE_SUCCESS_BELOW) {
            gout_push(out, GAME_OUT_MSG_INTIMIDATE_SUCCESS, 0, 0, 0, 0, 0);
            game_set_mode_explore(game);
        } else {
            gout_push(out, GAME_OUT_MSG_INTIMIDATE_FAIL, 0, 0, 0, 0, 0);
            combat_start(game, out);
        }
        return 1;
    }
    gout_push(out, GAME_OUT_MSG_PICK_123, 0, 0, 0, 0, 0);
    return 1;
}
