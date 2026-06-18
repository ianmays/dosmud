#include <stdlib.h>
#include "combat.h"
#include "game.h"
#include "gout.h"
#include "invent.h"
#include "items.h"
#include "gprog.h"
#include "npc.h"

/*
 * combat.c resolves the short battle loop only: it applies a reply, advances
 * the enemy turn, and converts a defeat into corpse state plus XP.
 * #159: queues GAME_EVENT_COMBAT phases; grendr maps them to combat copy.
 */

/*
 * #159: typed combat phases (payload layout in gout.h). phase -> arg0;
 * val0/val1/val2 -> arg1/arg2/arg3; grendr render_combat_event mirrors this.
 */
static void push_combat_phase(GameEventQueue *out, int phase,
                              int val0, int val1, int val2)
{
    game_event_push(out, GAME_EVENT_COMBAT, phase, val0, val1, val2, 0);
}

/* combat.enemy_level snapshot from combat_start; 1 when unset (harness fixtures). */
int combat_enemy_level(const struct GameState *game)
{
    if (game->combat.enemy_level > 0) {
        return game->combat.enemy_level;
    }
    return 1;
}

static void combat_end_active_enemy_encounter(struct GameState *game)
{
    int slot;

    slot = npc_find_by_dialogue(game, DIALOGUE_ENEMY);
    if (slot < 0) {
        return;
    }
    npc_end_encounter(game, game->npcs[slot].actor);
}

/*
 * Defeat loot RNG (combat-owned): one count roll (0-3 drops), then one item-tier
 * roll per drop into invent corpse slots via game_corpse_try_add. Zero drops
 * still leave corpse_present set for the stripped-body loot path.
 */
static int combat_roll_corpse_item(struct GameState *game)
{
    int roll;

    roll = game_roll_percent(game);
    if (roll < CFG_COMBAT_CORPSE_LOOT_SPEAR_BELOW) {
        return ITEM_SPEAR;
    }
    if (roll < CFG_COMBAT_CORPSE_LOOT_STICK_BELOW) {
        return ITEM_STICK;
    }
    if (roll < CFG_COMBAT_CORPSE_LOOT_BERRY_BELOW) {
        return ITEM_BERRY;
    }
    if (roll < CFG_COMBAT_CORPSE_LOOT_HERB_BELOW) {
        return ITEM_HERB;
    }
    return ITEM_FISH;
}

static int combat_roll_corpse_item_count(struct GameState *game)
{
    int roll;

    roll = game_roll_percent(game);
    if (roll < CFG_COMBAT_CORPSE_LOOT_NONE_BELOW) {
        return 0;
    }
    if (roll < CFG_COMBAT_CORPSE_LOOT_ONE_BELOW) {
        return 1;
    }
    if (roll < CFG_COMBAT_CORPSE_LOOT_TWO_BELOW) {
        return 2;
    }
    return 3;
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

static void combat_enemy_turn(struct GameState *game, GameEventQueue *out)
{
    int dmg;
    int level;

    level = combat_enemy_level(game);
    /* The enemy turn happens only after a non-terminal player reply. */
    dmg = CFG_COMBAT_ENEMY_DMG_BASE +
        ((level - 1) * CFG_COMBAT_ENEMY_DMG_PER_LEVEL) +
        game_roll_spread(game, CFG_COMBAT_ENEMY_DMG_SPREAD);
    if (game->combat.defending) {
        dmg -= CFG_COMBAT_DEFEND_DAMAGE_REDUCTION;
        if (dmg < 0) dmg = 0;
    }
    if (dmg > 0) {
        game->player_hp -= dmg;
    }
    push_combat_phase(out, GAME_COMBAT_PHASE_ENEMY_DAMAGE, dmg, 0, 0);
    if (game->player_hp <= 0) {
        game->player_hp = 0;
        push_combat_phase(out, GAME_COMBAT_PHASE_PLAYER_DOWN, 0, 0, 0);
        game->running = 0;
        return;
    }
    push_combat_phase(out, GAME_COMBAT_PHASE_STATUS,
        game->player_hp, game->combat.enemy_hp, level);
}

void combat_start(struct GameState *game, GameEventQueue *out)
{
    int slot;
    int level;

    /* npc.c owns roster level; combat snapshots it for scaling and save/load. */
    slot = npc_find_by_dialogue(game, DIALOGUE_ENEMY);
    level = 1;
    if (slot >= 0 && game->npcs[slot].level > 0) {
        level = game->npcs[slot].level;
    }
    game_set_mode_combat(game);
    game->combat.enemy_level = level;
    game->combat.enemy_hp = CFG_COMBAT_ENEMY_HP_BASE +
        ((level - 1) * CFG_COMBAT_ENEMY_HP_PER_LEVEL) +
        game_roll_spread(game, CFG_COMBAT_ENEMY_HP_SPREAD);
    game->combat.defending = 0;
    push_combat_phase(out, GAME_COMBAT_PHASE_START,
        game->player_hp, game->combat.enemy_hp, level);
}

void combat_resolve_reply(struct GameState *game, int choice, GameEventQueue *out)
{
    int dmg;
    int enemy_level;

    enemy_level = combat_enemy_level(game);
    if (choice == 1) {
        dmg = CFG_COMBAT_PLAYER_HIT_BASE +
            game_roll_spread(game, CFG_COMBAT_PLAYER_HIT_SPREAD) +
            combat_player_attack_bonus(game);
        game->combat.enemy_hp -= dmg;
        push_combat_phase(out, GAME_COMBAT_PHASE_PLAYER_DAMAGE, dmg, 0, 0);
    } else if (choice == 2) {
        game->combat.defending = 1;
        push_combat_phase(out, GAME_COMBAT_PHASE_BRACED, 0, 0, 0);
    } else if (choice == 3) {
        if (game_inv_bag_find_index(game, ITEM_SALVE) < 0) {
            push_combat_phase(out, GAME_COMBAT_PHASE_SALVE_NO_BAG, 0, 0, 0);
        } else {
            game_inv_bag_remove_item(game, ITEM_SALVE);
            if (game_heal_player(game, CFG_SALVE_HEAL_AMOUNT)) {
                push_combat_phase(out, GAME_COMBAT_PHASE_SALVE_HEAL,
                    game->player_hp, 0, 0);
            } else {
                push_combat_phase(out, GAME_COMBAT_PHASE_SALVE_FULL, 0, 0, 0);
            }
        }
    } else {
        push_combat_phase(out, GAME_COMBAT_PHASE_INVALID_CHOICE, 0, 0, 0);
        return;
    }

    if (game->combat.enemy_hp <= 0) {
        /* Defeat is resolved immediately so corpse state is fixed on the same turn. */
        game->combat.enemy_hp = 0;
        /* Enemy teardown follows the active DIALOGUE_ENEMY slot, not a fixed actor id. */
        combat_end_active_enemy_encounter(game);
        game_set_mode_explore(game);
        push_combat_phase(out, GAME_COMBAT_PHASE_ENEMY_DEFEATED, 0, 0,
            enemy_level);
        /* Clear stale corpse slots before seeding the new defeat loot table. */
        game_corpse_clear(game, game->player.room_id);
        game->corpse_present[game->player.room_id] = 1;
        {
            int drop_count;
            int slot;

            drop_count = combat_roll_corpse_item_count(game);
            /* One item-tier draw per slot before the kill XP spread roll. */
            for (slot = 0; slot < drop_count; ++slot) {
                (void)game_corpse_try_add(game, game->player.room_id,
                    combat_roll_corpse_item(game));
            }
        }
        progression_gain_enemy_xp(game, enemy_level,
            game_roll_spread(game, CFG_COMBAT_KILL_XP_SPREAD), out);
        return;
    }

    combat_enemy_turn(game, out);
    game->combat.defending = 0;
    if (game->running && game->mode == GAME_MODE_COMBAT) {
        push_combat_phase(out, GAME_COMBAT_PHASE_MENU, 0, 0, 0);
    }
}
