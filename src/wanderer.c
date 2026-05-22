#include <stdlib.h>
#include "wanderer.h"
#include "game.h"
#include "grendr.h"

void wanderer_update_separation(struct GameState *game)
{
    if (game->player.room_id != game->wanderer_room) {
        game->wanderer_need_separation = 0;
    }
}

void wanderer_step(struct GameState *game)
{
    struct Room *r;
    int dirs[CFG_DIR_MAX];
    int n;
    int i;
    int pick;

    if (game->world.room_count <= 0) {
        return;
    }
    if (game->wanderer_room < 0 || game->wanderer_room >= game->world.room_count) {
        return;
    }
    r = &game->world.rooms[game->wanderer_room];
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
    pick = rand() % n;
    game->wanderer_room = r->exits[dirs[pick]];
}

void wanderer_begin_encounter(struct GameState *game)
{
    if (game_is_busy_dialogue(game)) {
        return;
    }
    if (game->wanderer_need_separation) {
        return;
    }
    render_wanderer_scene();
    game_set_mode_dialogue(game, DIALOGUE_WANDERER);
    game->wanderer_need_separation = 1;
}

void wanderer_apply_reply(int choice)
{
    render_wanderer_reply(choice);
}

int wanderer_cmd_reply(struct GameState *game, int choice)
{
    if (choice < 1 || choice > 3) {
        render_msg_pick_123();
        return 1;
    }
    wanderer_apply_reply(choice);
    game_set_mode_explore(game);
    game->wanderer_active = 0;
    game->wanderer_room = -1;
    game->wanderer_return_tick = game->tick + CFG_WANDERER_RETURN_DELAY_BASE +
        (rand() % CFG_WANDERER_RETURN_DELAY_SPREAD);
    return 1;
}
