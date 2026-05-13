#include <stdlib.h>
#include "combat.h"
#include "game.h"
#include "grendr.h"
#include "invent.h"
#include "items.h"
#include "gprog.h"

static void combat_enemy_turn(struct GameState *game)
{
    int dmg;
    dmg = CFG_COMBAT_ENEMY_DMG_BASE + (rand() % CFG_COMBAT_ENEMY_DMG_SPREAD);
    if (game->combat_defending) {
        dmg -= CFG_COMBAT_DEFEND_DAMAGE_REDUCTION;
        if (dmg < 0) dmg = 0;
    }
    if (dmg > 0) {
        game->player_hp -= dmg;
    }
    render_combat_enemy_strike(dmg);
    if (game->player_hp <= 0) {
        game->player_hp = 0;
        render_combat_player_fallen();
        game->running = 0;
        return;
    }
    render_combat_status_line(game->player_hp, game->enemy_hp);
}

void combat_start(struct GameState *game)
{
    game->enemy_dialogue = 0;
    game->enemy_handover_pick = 0;
    game->combat_active = 1;
    game->enemy_hp = CFG_COMBAT_ENEMY_HP_BASE + (rand() % CFG_COMBAT_ENEMY_HP_SPREAD);
    game->combat_defending = 0;
    render_combat_start(game->player_hp, game->enemy_hp);
}

void combat_resolve_reply(struct GameState *game, int choice)
{
    int dmg;
    if (choice == 1) {
        dmg = CFG_COMBAT_PLAYER_HIT_BASE + (rand() % CFG_COMBAT_PLAYER_HIT_SPREAD) +
            game->damage_bonus;
        if (game->weapon_equipped != ITEM_NONE) {
            dmg += item_weapon_damage_bonus(game->weapon_equipped);
        }
        game->enemy_hp -= dmg;
        render_combat_player_hit(dmg);
    } else if (choice == 2) {
        game->combat_defending = 1;
        render_combat_braced();
    } else if (choice == 3) {
        if (game_inv_bag_find_index(game, ITEM_SALVE) < 0) {
            render_combat_no_salve_bag();
        } else {
            game_inv_bag_remove_item(game, ITEM_SALVE);
            game->player_hp += CFG_SALVE_HEAL_AMOUNT;
            if (game->player_hp > game->max_hp) game->player_hp = game->max_hp;
            render_combat_salve_in_combat(game->player_hp);
        }
    } else {
        render_combat_invalid_choice();
        return;
    }

    if (game->enemy_hp <= 0) {
        game->enemy_hp = 0;
        game->combat_active = 0;
        game->combat_defending = 0;
        render_combat_bandit_defeated();
        game->corpse_present[game->player.room_id] = 1;
        {
            int roll;
            int loot_item;

            roll = rand() % CFG_ROLL_PERCENT_RANGE;
            if (roll < CFG_COMBAT_CORPSE_LOOT_SPEAR_BELOW) {
                loot_item = ITEM_SPEAR;
            } else if (roll < CFG_COMBAT_CORPSE_LOOT_STICK_BELOW) {
                loot_item = ITEM_STICK;
            } else if (roll < CFG_COMBAT_CORPSE_LOOT_BERRY_BELOW) {
                loot_item = ITEM_BERRY;
            } else if (roll < CFG_COMBAT_CORPSE_LOOT_HERB_BELOW) {
                loot_item = ITEM_HERB;
            } else {
                loot_item = ITEM_FISH;
            }
            game->corpse_loot[game->player.room_id] = loot_item;
        }
        progression_gain_xp(game, CFG_COMBAT_KILL_XP_BASE + (rand() % CFG_COMBAT_KILL_XP_SPREAD));
        return;
    }

    combat_enemy_turn(game);
    game->combat_defending = 0;
    if (game->running && game->combat_active) {
        render_combat_menu();
    }
}
