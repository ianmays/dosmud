#include "genc.h"
#include "game.h"
#include "grendr.h"

void enemy_begin_encounter(struct GameState *game)
{
    if (game_is_busy_dialogue(game)) {
        return;
    }
    render_bandit_encounter_open();
    game_set_mode_dialogue(game, DIALOGUE_ENEMY);
}
