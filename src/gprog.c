#include "gprog.h"
#include "game.h"
#include "grendr.h"

/* Experience progression is centralized so level thresholds and reward side effects stay in sync. */

int game_xp_to_next_level(int level)
{
    return CFG_XP_LEVEL_BASE + ((level - 1) * CFG_XP_LEVEL_PER_LEVEL);
}

void progression_gain_xp(struct GameState *game, int amount)
{
    int needed;
    /* Multiple level-ups can happen in one grant, so the threshold is recomputed each loop. */
    game->xp += amount;
    render_xp_gained(amount);
    needed = game_xp_to_next_level(game->level);
    while (game->xp >= needed) {
        game->xp -= needed;
        game->level += 1;
        game->max_hp += CFG_LEVELUP_MAX_HP_DELTA;
        game->damage_bonus += CFG_LEVELUP_DAMAGE_BONUS_DELTA;
        if (game->bag_capacity < CFG_BAG_MAX) {
            game->bag_capacity += CFG_LEVELUP_BAG_CAPACITY_DELTA;
        }
        game->player_hp = game->max_hp;
        render_level_up(game->level, game->max_hp, game->damage_bonus,
            game->bag_capacity);
        needed = game_xp_to_next_level(game->level);
    }
}
