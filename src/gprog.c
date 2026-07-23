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

u32 progression_cumulative_xp(int level, int xp)
{
    u32 total;
    int current_level;

    total = xp > 0 ? (u32)xp : 0UL;
    for (current_level = CFG_START_LEVEL;
            current_level < level; ++current_level) {
        total += (u32)game_xp_to_next_level(current_level);
    }
    return total;
}

void progression_rebuild_from_cumulative_xp(struct GameState *game,
                                             u32 cumulative_xp)
{
    u32 needed;
    int levels_gained;

    game->level = CFG_START_LEVEL;
    needed = (u32)game_xp_to_next_level(game->level);
    while (cumulative_xp >= needed) {
        cumulative_xp -= needed;
        game->level += 1;
        needed = (u32)game_xp_to_next_level(game->level);
    }
    game->xp = (int)cumulative_xp;
    levels_gained = game->level - CFG_START_LEVEL;
    game->max_hp = CFG_START_MAX_HP +
        (levels_gained * CFG_LEVELUP_MAX_HP_DELTA);
    game->damage_bonus = CFG_START_DAMAGE_BONUS +
        (levels_gained * CFG_LEVELUP_DAMAGE_BONUS_DELTA);
    game->bag_capacity = CFG_START_BAG_CAPACITY +
        (levels_gained * CFG_LEVELUP_BAG_CAPACITY_DELTA);
    if (game->bag_capacity > CFG_BAG_MAX) {
        game->bag_capacity = CFG_BAG_MAX;
    }
}

/*
 * Mutates level and derived stats only; game.c emits GAME_EVENT_PLAYER_DEFEAT
 * rather than XP_GAIN/STAT_CHANGE so camp copy stays on the defeat path.
 * Integer percent avoids float and keeps loss deterministic for the same total.
 */
u32 progression_apply_defeat_penalty(struct GameState *game)
{
    u32 total;
    u32 loss;

    total = progression_cumulative_xp(game->level, game->xp);
    loss = ((total / 100UL) * (u32)CFG_PLAYER_DEFEAT_XP_PERCENT) +
        (((total % 100UL) * (u32)CFG_PLAYER_DEFEAT_XP_PERCENT) / 100UL);
    progression_rebuild_from_cumulative_xp(game, total - loss);
    return loss;
}
