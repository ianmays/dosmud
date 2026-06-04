#include <stdlib.h>
#include "combat.h"
#include "game.h"
#include "gout.h"
#include "invent.h"
#include "items.h"
#include "gprog.h"

/*
 * combat.c resolves the short battle loop only: it applies a reply, advances
 * the enemy turn, and converts a defeat into corpse state plus XP.
 * #159: queues GAME_EVENT_COMBAT phases; grendr maps them to combat copy.
 */

static void push_combat_phase(struct GameOutput *out, int phase,
                              int arg0, int arg1)
{
    game_event_push(out, GAME_EVENT_COMBAT, phase, arg0, arg1, 0, 0);
}

int combat_player_attack_bonus(const struct GameState *game)
{
    int bonus;

    bonus = game->damage_bonus;
    if (game->weapon_equipped != ITEM_NONE) {
        bonus += item_weapon_damage_bonus(game->weapon_equipped);
    }
    return bonus;
}

static void combat_enemy_turn(struct GameState *game, struct GameOutput *out)
{
    int dmg;
    /* The enemy turn happens only after a non-terminal player reply. */
    dmg = CFG_COMBAT_ENEMY_DMG_BASE + game_roll_spread(game, CFG_COMBAT_ENEMY_DMG_SPREAD);
    if (game->combat.defending) {
        dmg -= CFG_COMBAT_DEFEND_DAMAGE_REDUCTION;
        if (dmg < 0) dmg = 0;
    }
    if (dmg > 0) {
        game->player_hp -= dmg;
    }
    push_combat_phase(out, GAME_COMBAT_PHASE_ENEMY_DAMAGE, dmg, 0);
    if (game->player_hp <= 0) {
        game->player_hp = 0;
        push_combat_phase(out, GAME_COMBAT_PHASE_PLAYER_DOWN, 0, 0);
        game->running = 0;
        return;
    }
    push_combat_phase(out, GAME_COMBAT_PHASE_STATUS,
        game->player_hp, game->combat.enemy_hp);
}

void combat_start(struct GameState *game, struct GameOutput *out)
{
    game_set_mode_combat(game);
    game->combat.enemy_hp = CFG_COMBAT_ENEMY_HP_BASE +
        game_roll_spread(game, CFG_COMBAT_ENEMY_HP_SPREAD);
    game->combat.defending = 0;
    push_combat_phase(out, GAME_COMBAT_PHASE_START,
        game->player_hp, game->combat.enemy_hp);
}

void combat_resolve_reply(struct GameState *game, int choice, struct GameOutput *out)
{
    int dmg;
    if (choice == 1) {
        dmg = CFG_COMBAT_PLAYER_HIT_BASE +
            game_roll_spread(game, CFG_COMBAT_PLAYER_HIT_SPREAD) +
            combat_player_attack_bonus(game);
        game->combat.enemy_hp -= dmg;
        push_combat_phase(out, GAME_COMBAT_PHASE_PLAYER_DAMAGE, dmg, 0);
    } else if (choice == 2) {
        game->combat.defending = 1;
        push_combat_phase(out, GAME_COMBAT_PHASE_BRACED, 0, 0);
    } else if (choice == 3) {
        if (game_inv_bag_find_index(game, ITEM_SALVE) < 0) {
            push_combat_phase(out, GAME_COMBAT_PHASE_SALVE_NO_BAG, 0, 0);
        } else {
            game_inv_bag_remove_item(game, ITEM_SALVE);
            if (game_heal_player(game, CFG_SALVE_HEAL_AMOUNT)) {
                push_combat_phase(out, GAME_COMBAT_PHASE_SALVE_HEAL,
                    game->player_hp, 0);
            } else {
                push_combat_phase(out, GAME_COMBAT_PHASE_SALVE_FULL, 0, 0);
            }
        }
    } else {
        push_combat_phase(out, GAME_COMBAT_PHASE_INVALID_CHOICE, 0, 0);
        return;
    }

    if (game->combat.enemy_hp <= 0) {
        /* Defeat is resolved immediately so corpse state is fixed on the same turn. */
        game->combat.enemy_hp = 0;
        game_set_mode_explore(game);
        push_combat_phase(out, GAME_COMBAT_PHASE_ENEMY_DEFEATED, 0, 0);
        game->corpse_present[game->player.room_id] = 1;
        {
            int roll;
            int loot_item;

            roll = game_roll_percent(game);
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
        progression_gain_xp(game, CFG_COMBAT_KILL_XP_BASE +
            game_roll_spread(game, CFG_COMBAT_KILL_XP_SPREAD), out);
        return;
    }

    combat_enemy_turn(game, out);
    game->combat.defending = 0;
    if (game->running && game->mode == GAME_MODE_COMBAT) {
        push_combat_phase(out, GAME_COMBAT_PHASE_MENU, 0, 0);
    }
}
