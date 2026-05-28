/*
 * Inventory and ground-slot ownership live here; the module keeps bag, hand,
 * and room storage explicit so the DOS-sized buffers stay predictable.
 */

#include "config.h"
#include "invent.h"
#include "game.h"
#include "gout.h"
#include "items.h"

int game_room_ground_try_add(struct GameState *game, int room_id, int item_id)
{
    int s;
    for (s = 0; s < CFG_AREA_ITEM_SLOTS; ++s) {
        if (game->room_item[room_id][s] == ITEM_NONE) {
            game->room_item[room_id][s] = item_id;
            return 1;
        }
    }
    return 0;
}

int game_room_ground_has_space(struct GameState *game, int room_id)
{
    int s;
    for (s = 0; s < CFG_AREA_ITEM_SLOTS; ++s) {
        if (game->room_item[room_id][s] == ITEM_NONE) {
            return 1;
        }
    }
    return 0;
}

static int room_ground_is_empty(struct GameState *game, int room_id)
{
    int s;
    for (s = 0; s < CFG_AREA_ITEM_SLOTS; ++s) {
        if (game->room_item[room_id][s] != ITEM_NONE) {
            return 0;
        }
    }
    return 1;
}

static int room_find_item_slot(struct GameState *game, int room_id, int item_id)
{
    int s;
    for (s = 0; s < CFG_AREA_ITEM_SLOTS; ++s) {
        if (game->room_item[room_id][s] == item_id) {
            return s;
        }
    }
    return -1;
}

static int room_first_free_slot(struct GameState *game, int room_id)
{
    int s;
    for (s = 0; s < CFG_AREA_ITEM_SLOTS; ++s) {
        if (game->room_item[room_id][s] == ITEM_NONE) {
            return s;
        }
    }
    return -1;
}

static void room_remove_slot_compact(struct GameState *game, int room_id, int slot)
{
    int s;
    /* Compact after removal so ground slots stay dense and deterministic. */
    for (s = slot; s < CFG_AREA_ITEM_SLOTS - 1; ++s) {
        game->room_item[room_id][s] = game->room_item[room_id][s + 1];
    }
    game->room_item[room_id][CFG_AREA_ITEM_SLOTS - 1] = ITEM_NONE;
}

int game_inv_bag_find_index(struct GameState *game, int item_id)
{
    int i;
    for (i = 0; i < game->bag_count; ++i) {
        if (game->bag[i] == item_id) {
            return i;
        }
    }
    return -1;
}

int game_inv_player_has_item(struct GameState *game, int item_id)
{
    if (game_inv_bag_find_index(game, item_id) >= 0) {
        return 1;
    }
    if (game->weapon_equipped == item_id) {
        return 1;
    }
    return 0;
}

/* Remove bag slot without clearing weapon_equipped (used when moving item to hand). */
static int game_inv_bag_remove_index_transfer(struct GameState *game, int index)
{
    int i;
    /* Dropping or wielding can move an item out of the bag without losing ownership. */
    if (index < 0 || index >= game->bag_count) {
        return 0;
    }
    for (i = index; i < game->bag_count - 1; ++i) {
        game->bag[i] = game->bag[i + 1];
    }
    game->bag_count -= 1;
    game->bag[game->bag_count] = ITEM_NONE;
    return 1;
}

int game_inv_bag_add(struct GameState *game, int item_id)
{
    if (game->bag_count >= game->bag_capacity) {
        return 0;
    }
    game->bag[game->bag_count] = item_id;
    game->bag_count += 1;
    return 1;
}

int game_inv_bag_remove_index(struct GameState *game, int index)
{
    int i;
    int removed;
    if (index < 0 || index >= game->bag_count) {
        return 0;
    }
    removed = game->bag[index];
    if (game->weapon_equipped == removed) {
        game->weapon_equipped = ITEM_NONE;
    }
    for (i = index; i < game->bag_count - 1; ++i) {
        game->bag[i] = game->bag[i + 1];
    }
    game->bag_count -= 1;
    game->bag[game->bag_count] = ITEM_NONE;
    return 1;
}

int game_inv_bag_remove_item(struct GameState *game, int item_id)
{
    int idx;
    idx = game_inv_bag_find_index(game, item_id);
    if (idx < 0) {
        return 0;
    }
    return game_inv_bag_remove_index(game, idx);
}

int game_inv_cmd_loot(struct GameState *game, struct GameOutput *out)
{
    int room_id;
    int ground_item;

    room_id = game->player.room_id;
    if (!game->corpse_present[room_id]) {
        gout_push(out, GAME_OUT_INV_NO_BODY_LOOT, 0, 0, 0, 0, 0);
        return 1;
    }
    ground_item = game->corpse_loot[room_id];
    if (ground_item == ITEM_NONE) {
        gout_push(out, GAME_OUT_INV_BODY_STRIPPED, 0, 0, 0, 0, 0);
        return 1;
    }
    if (!game_inv_bag_add(game, ground_item)) {
        gout_push(out, GAME_OUT_INV_BAG_FULL_DROP, 0, 0, 0, 0, 0);
        return 1;
    }
    gout_push(out, GAME_OUT_INV_LOOT, 0, 0, 0, 0, item_name(ground_item));
    game->corpse_loot[room_id] = ITEM_NONE;
    game->corpse_present[room_id] = 0;
    return 1;
}

int game_inv_cmd_take(struct GameState *game, int item_arg, struct GameOutput *out)
{
    int room_id;
    int ground_item;
    int slot;

    if (game->mode == GAME_MODE_COMBAT) {
        gout_push(out, GAME_OUT_INV_NO_RUMMAGE_COMBAT, 0, 0, 0, 0, 0);
        return 1;
    }
    room_id = game->player.room_id;
    if (room_ground_is_empty(game, room_id)) {
        gout_push(out, GAME_OUT_INV_TAKE_NOTHING, 0, 0, 0, 0, 0);
        return 1;
    }
    slot = room_find_item_slot(game, room_id, item_arg);
    if (slot < 0) {
        gout_push(out, GAME_OUT_INV_CANNOT_TAKE_HERE, 0, 0, 0, 0, 0);
        return 1;
    }
    ground_item = game->room_item[room_id][slot];
    if (!game_inv_bag_add(game, ground_item)) {
        gout_push(out, GAME_OUT_INV_BAG_FULL, game->bag_capacity, 0, 0, 0, 0);
        return 1;
    }
    room_remove_slot_compact(game, room_id, slot);
    gout_push(out, GAME_OUT_INV_PICKUP, 0, 0, 0, 0, item_name(ground_item));
    return 1;
}

int game_inv_cmd_take_all(struct GameState *game, struct GameOutput *out)
{
    int room_id;
    int ground_count;
    int ground_items[CFG_AREA_ITEM_SLOTS];
    int slot;
    int i;

    if (game->mode == GAME_MODE_COMBAT) {
        gout_push(out, GAME_OUT_INV_NO_RUMMAGE_COMBAT, 0, 0, 0, 0, 0);
        return 1;
    }
    room_id = game->player.room_id;
    ground_count = 0;
    for (slot = 0; slot < CFG_AREA_ITEM_SLOTS; ++slot) {
        if (game->room_item[room_id][slot] != ITEM_NONE) {
            ground_items[ground_count] = game->room_item[room_id][slot];
            ground_count += 1;
        }
    }
    if (ground_count == 0) {
        gout_push(out, GAME_OUT_INV_TAKE_NOTHING, 0, 0, 0, 0, 0);
        return 1;
    }
    if (game->bag_count + ground_count > game->bag_capacity) {
        gout_push(out, GAME_OUT_INV_BAG_FULL, game->bag_capacity, 0, 0, 0, 0);
        return 1;
    }
    for (i = 0; i < ground_count; ++i) {
        game_inv_bag_add(game, ground_items[i]);
    }
    for (slot = 0; slot < CFG_AREA_ITEM_SLOTS; ++slot) {
        game->room_item[room_id][slot] = ITEM_NONE;
    }
    for (i = 0; i < ground_count; ++i) {
        gout_push(out, GAME_OUT_INV_PICKUP, 0, 0, 0, 0, item_name(ground_items[i]));
    }
    return 1;
}

int game_inv_cmd_drop(struct GameState *game, int item_arg, struct GameOutput *out)
{
    int room_id;
    int slot;

    if (game->mode == GAME_MODE_COMBAT) {
        gout_push(out, GAME_OUT_INV_NO_DROP_COMBAT, 0, 0, 0, 0, 0);
        return 1;
    }
    room_id = game->player.room_id;
    if (!game_inv_player_has_item(game, item_arg)) {
        gout_push(out, GAME_OUT_INV_NOT_CARRYING, 0, 0, 0, 0, item_name(item_arg));
        return 1;
    }
    slot = room_first_free_slot(game, room_id);
    if (slot < 0) {
        gout_push(out, GAME_OUT_INV_GROUND_FULL, CFG_AREA_ITEM_SLOTS, 0, 0, 0, 0);
        return 1;
    }
    {
        int idx;
        idx = game_inv_bag_find_index(game, item_arg);
        if (idx >= 0) {
            if (!game_inv_bag_remove_index_transfer(game, idx)) {
                return 1;
            }
        } else if (game->weapon_equipped == item_arg) {
            game->weapon_equipped = ITEM_NONE;
        } else {
            gout_push(out, GAME_OUT_INV_NOT_CARRYING, 0, 0, 0, 0, item_name(item_arg));
            return 1;
        }
    }
    game->room_item[room_id][slot] = item_arg;
    gout_push(out, GAME_OUT_INV_DROP, 0, 0, 0, 0, item_name(item_arg));
    return 1;
}

int game_inv_cmd_bag(struct GameState *game, struct GameOutput *out)
{
    (void)game;
    gout_push(out, GAME_OUT_INV_BAG, 0, 0, 0, 0, 0);
    return 1;
}

int game_inv_cmd_eat(struct GameState *game, int item_arg, struct GameOutput *out)
{
    if (game->mode == GAME_MODE_COMBAT) {
        gout_push(out, GAME_OUT_INV_NO_EAT_COMBAT, 0, 0, 0, 0, 0);
        return 1;
    }
    if (game_inv_bag_find_index(game, item_arg) < 0) {
        gout_push(out, GAME_OUT_INV_NOT_CARRYING, 0, 0, 0, 0, item_name(item_arg));
        return 1;
    }
    if (!item_is_edible(item_arg)) {
        gout_push(out, GAME_OUT_INV_CANNOT_EAT, 0, 0, 0, 0, item_name(item_arg));
        return 1;
    }
    game_inv_bag_remove_item(game, item_arg);
    if (!game_heal_player(game, item_food_heal_amount(item_arg))) {
        if (item_arg == ITEM_BERRY) {
            gout_push(out, GAME_OUT_INV_EAT_BERRY_FULL, 0, 0, 0, 0, 0);
        } else {
            gout_push(out, GAME_OUT_INV_EAT_FISH_FULL, 0, 0, 0, 0, 0);
        }
    } else if (item_arg == ITEM_BERRY) {
        gout_push(out, GAME_OUT_INV_EAT_BERRY_HEALED, game->player_hp, 0, 0, 0, 0);
    } else {
        gout_push(out, GAME_OUT_INV_EAT_FISH_HEALED, game->player_hp, 0, 0, 0, 0);
    }
    return 1;
}

int game_inv_cmd_use(struct GameState *game, int item_arg, struct GameOutput *out)
{
    if (game->mode == GAME_MODE_COMBAT) {
        gout_push(out, GAME_OUT_INV_USE_REPLY_COMBAT, 0, 0, 0, 0, 0);
        return 1;
    }
    if (game_inv_bag_find_index(game, item_arg) < 0) {
        gout_push(out, GAME_OUT_INV_NOT_CARRYING, 0, 0, 0, 0, item_name(item_arg));
        return 1;
    }
    if (item_arg == ITEM_TORCH) {
        gout_push(out, GAME_OUT_INV_USE_TORCH, 0, 0, 0, 0, 0);
        return 1;
    }
    if (item_arg == ITEM_SALVE) {
        game_inv_bag_remove_item(game, item_arg);
        if (game_heal_player(game, CFG_SALVE_HEAL_AMOUNT)) {
            gout_push(out, GAME_OUT_INV_USE_SALVE, game->player_hp, 0, 0, 0, 0);
        } else {
            gout_push(out, GAME_OUT_INV_USE_SALVE_FULL, 0, 0, 0, 0, 0);
        }
        return 1;
    }
    if (item_arg == ITEM_SPEAR) {
        gout_push(out, GAME_OUT_INV_USE_SPEAR, 0, 0, 0, 0, 0);
        return 1;
    }
    gout_push(out, GAME_OUT_INV_NO_USE, 0, 0, 0, 0, item_name(item_arg));
    return 1;
}

/* Remove one ingredient from bag if present, else from the wielded slot. */
static int craft_consume_one(struct GameState *game, int item_id)
{
    if (game_inv_bag_find_index(game, item_id) >= 0) {
        return game_inv_bag_remove_item(game, item_id);
    }
    if (game->weapon_equipped == item_id) {
        game->weapon_equipped = ITEM_NONE;
        return 1;
    }
    return 0;
}

int game_inv_cmd_craft(struct GameState *game, int item_arg, struct GameOutput *out)
{
    if (game->mode == GAME_MODE_COMBAT) {
        gout_push(out, GAME_OUT_INV_NO_CRAFT_COMBAT, 0, 0, 0, 0, 0);
        return 1;
    }
    if (item_arg == ITEM_TORCH) {
        if (!game_inv_player_has_item(game, ITEM_STICK) ||
                !game_inv_player_has_item(game, ITEM_REED)) {
            gout_push(out, GAME_OUT_INV_NEED_TORCH, 0, 0, 0, 0, 0);
            return 1;
        }
        craft_consume_one(game, ITEM_STICK);
        craft_consume_one(game, ITEM_REED);
        game_inv_bag_add(game, ITEM_TORCH);
        gout_push(out, GAME_OUT_INV_CRAFT_TORCH, 0, 0, 0, 0, 0);
        return 1;
    }
    if (item_arg == ITEM_SALVE) {
        if (!game_inv_player_has_item(game, ITEM_HERB) ||
                !game_inv_player_has_item(game, ITEM_BERRY)) {
            gout_push(out, GAME_OUT_INV_NEED_SALVE, 0, 0, 0, 0, 0);
            return 1;
        }
        craft_consume_one(game, ITEM_HERB);
        craft_consume_one(game, ITEM_BERRY);
        game_inv_bag_add(game, ITEM_SALVE);
        gout_push(out, GAME_OUT_INV_CRAFT_SALVE, 0, 0, 0, 0, 0);
        return 1;
    }
    if (item_arg == ITEM_SPEAR) {
        if (!game_inv_player_has_item(game, ITEM_STICK) ||
                !game_inv_player_has_item(game, ITEM_STONE)) {
            gout_push(out, GAME_OUT_INV_NEED_SPEAR, 0, 0, 0, 0, 0);
            return 1;
        }
        craft_consume_one(game, ITEM_STICK);
        craft_consume_one(game, ITEM_STONE);
        game_inv_bag_add(game, ITEM_SPEAR);
        gout_push(out, GAME_OUT_INV_CRAFT_SPEAR, 0, 0, 0, 0, 0);
        return 1;
    }
    gout_push(out, GAME_OUT_INV_CRAFT_UNKNOWN, 0, 0, 0, 0, 0);
    return 1;
}

int game_inv_cmd_wield(struct GameState *game, int item_arg, struct GameOutput *out)
{
    int idx;
    int old_weapon;

    if (item_arg == game->weapon_equipped) {
        gout_push(out, GAME_OUT_INV_ALREADY_WIELDING, 0, 0, 0, 0, item_name(item_arg));
        return 1;
    }
    idx = game_inv_bag_find_index(game, item_arg);
    if (idx < 0) {
        gout_push(out, GAME_OUT_INV_NOT_CARRYING, 0, 0, 0, 0, item_name(item_arg));
        return 1;
    }
    if (!item_is_weapon(item_arg)) {
        gout_push(out, GAME_OUT_INV_WIELD_NOT_WEAPON, 0, 0, 0, 0, 0);
        return 1;
    }
    old_weapon = game->weapon_equipped;
    if (!game_inv_bag_remove_index_transfer(game, idx)) {
        return 1;
    }
    if (old_weapon != ITEM_NONE) {
        if (!game_inv_bag_add(game, old_weapon)) {
            if (!game_inv_bag_add(game, item_arg)) {
                game->weapon_equipped = old_weapon;
                return 1;
            }
            game->weapon_equipped = old_weapon;
            gout_push(out, GAME_OUT_INV_WIELD_STOW_FAIL, 0, 0, 0, 0, 0);
            return 1;
        }
    }
    game->weapon_equipped = item_arg;
    gout_push(out, GAME_OUT_INV_WIELD, 0, 0, 0, 0, item_name(item_arg));
    return 1;
}

int game_inv_cmd_unwield(struct GameState *game, struct GameOutput *out)
{
    int room_id;
    int slot;
    int w;

    if (game->weapon_equipped == ITEM_NONE) {
        gout_push(out, GAME_OUT_INV_UNWIELD_EMPTY, 0, 0, 0, 0, 0);
        return 1;
    }
    w = game->weapon_equipped;
    if (game_inv_bag_add(game, w)) {
        game->weapon_equipped = ITEM_NONE;
        gout_push(out, GAME_OUT_INV_UNWIELD, 0, 0, 0, 0, 0);
        return 1;
    }
    if (game->mode == GAME_MODE_COMBAT) {
        gout_push(out, GAME_OUT_INV_UNWIELD_CANNOT, 0, 0, 0, 0, 0);
        return 1;
    }
    room_id = game->player.room_id;
    slot = room_first_free_slot(game, room_id);
    if (slot < 0) {
        gout_push(out, GAME_OUT_INV_UNWIELD_CANNOT, 0, 0, 0, 0, 0);
        return 1;
    }
    game->weapon_equipped = ITEM_NONE;
    game->room_item[room_id][slot] = w;
    gout_push(out, GAME_OUT_INV_UNWIELD_GROUND, 0, 0, 0, 0, item_name(w));
    return 1;
}
