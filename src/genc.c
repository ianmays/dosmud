#include "genc.h"
#include "combat.h"
#include "game.h"
#include "grendr.h"
#include "invent.h"
#include "items.h"

void enemy_begin_encounter(struct GameState *game)
{
    if (game_is_busy_dialogue(game)) {
        return;
    }
    render_bandit_encounter_open();
    game_set_mode_dialogue(game, DIALOGUE_ENEMY);
}

int genc_cmd_give(struct GameState *game, int item_arg)
{
    if (game->mode == GAME_MODE_DIALOGUE &&
            game->dialogue == DIALOGUE_ENEMY &&
            game->enemy_handover_pick == 1) {
        if (!game_inv_player_has_item(game, item_arg)) {
            render_msg_bandit_give_not_carrying();
            return 1;
        }
        render_msg_hand_over_item(item_name(item_arg));
        if (game->weapon_equipped == item_arg) {
            game->weapon_equipped = ITEM_NONE;
        } else {
            game_inv_bag_remove_item(game, item_arg);
        }
        game_set_mode_explore(game);
        return 1;
    }
    render_msg_give_wrong_context();
    return 1;
}

int genc_cmd_reply(struct GameState *game, int choice)
{
    if (choice == 1) {
        combat_start(game);
        return 1;
    }
    if (choice == 2) {
        if (game->bag_count <= 0 && game->weapon_equipped == ITEM_NONE) {
            render_msg_bag_empty_bandit();
            combat_start(game);
            return 1;
        }
        game->enemy_handover_pick = 1;
        render_bandit_handover_pick_prompt();
        return 1;
    }
    if (choice == 3) {
        game->enemy_handover_pick = 0;
        if (game_roll_percent(game) < CFG_BANDIT_INTIMIDATE_SUCCESS_BELOW) {
            render_msg_intimidate_success();
            game_set_mode_explore(game);
        } else {
            render_msg_intimidate_fail();
            combat_start(game);
        }
        return 1;
    }
    render_msg_pick_123();
    return 1;
}
