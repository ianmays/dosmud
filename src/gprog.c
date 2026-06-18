#include "gprog.h"
#include "game.h"
#include "gout.h"

/*
 * Experience progression is centralized so level thresholds and reward side
 * effects stay in sync. #159: queues GAME_EVENT_XP_GAIN and STAT_CHANGE.
 */

int game_xp_to_next_level(int level)
{
    return CFG_XP_LEVEL_BASE + ((level - 1) * CFG_XP_LEVEL_PER_LEVEL);
}

/* Kill XP uses combat snapshot level; spread_roll is the combat defeat draw. */
int progression_enemy_xp_reward(int enemy_level, int spread_roll)
{
    int bonus_levels;

    bonus_levels = enemy_level - 1;
    if (bonus_levels < 0) {
        bonus_levels = 0;
    }
    return CFG_COMBAT_KILL_XP_BASE +
        (bonus_levels * CFG_COMBAT_KILL_XP_PER_LEVEL) +
        spread_roll;
}

void progression_gain_xp(struct GameState *game, int amount, GameEventQueue *out)
{
    int needed;
    /* Multiple level-ups can happen in one grant, so the threshold is recomputed each loop. */
    game->xp += amount;
    game_event_push(out, GAME_EVENT_XP_GAIN, amount, 0, 0, 0, 0);
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
        game_event_push(out, GAME_EVENT_STAT_CHANGE, game->level, game->max_hp,
            game->damage_bonus, game->bag_capacity, 0);
        needed = game_xp_to_next_level(game->level);
    }
}

/* combat.c calls this on defeat so level scaling stays out of combat_resolve_reply. */
void progression_gain_enemy_xp(struct GameState *game, int enemy_level,
                               int spread_roll, GameEventQueue *out)
{
    progression_gain_xp(game,
        progression_enemy_xp_reward(enemy_level, spread_roll), out);
}
