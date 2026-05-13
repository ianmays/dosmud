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
    game->pond_dialogue = 0;
    render_wanderer_scene();
    game->wanderer_dialogue = 1;
    game->wanderer_need_separation = 1;
}

void wanderer_apply_reply(int choice)
{
    render_wanderer_reply(choice);
}
