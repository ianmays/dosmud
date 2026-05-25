/*
 * Inventory and ground-slot ownership live here; the module keeps bag, hand,
 * and room storage explicit so the DOS-sized buffers stay predictable.
 */

#include "config.h"
#include "invent.h"
#include "game.h"
#include "grendr.h"
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

int game_inv_cmd_loot(struct GameState *game)
{
    int room_id;
    int ground_item;

    room_id = game->player.room_id;
    if (!game->corpse_present[room_id]) {
        render_inv_no_body_loot();
        return 1;
    }
    ground_item = game->corpse_loot[room_id];
    if (ground_item == ITEM_NONE) {
        render_inv_body_stripped();
        return 1;
    }
    if (!game_inv_bag_add(game, ground_item)) {
        render_inv_bag_full_drop();
        return 1;
    }
    render_inv_loot(item_name(ground_item));
    game->corpse_loot[room_id] = ITEM_NONE;
    game->corpse_present[room_id] = 0;
    return 1;
}

int game_inv_cmd_take(struct GameState *game, int item_arg)
{
    int room_id;
    int ground_item;
    int slot;

    if (game->mode == GAME_MODE_COMBAT) {
        render_inv_no_rummage_combat();
        return 1;
    }
    room_id = game->player.room_id;
    if (room_ground_is_empty(game, room_id)) {
        render_inv_take_nothing();
        return 1;
    }
    slot = room_find_item_slot(game, room_id, item_arg);
    if (slot < 0) {
        render_inv_cannot_take_here();
        return 1;
    }
    ground_item = game->room_item[room_id][slot];
    if (!game_inv_bag_add(game, ground_item)) {
        render_inv_bag_full(game->bag_capacity);
        return 1;
    }
    room_remove_slot_compact(game, room_id, slot);
    render_inv_pickup(item_name(ground_item));
    return 1;
}

int game_inv_cmd_take_all(struct GameState *game)
{
    int room_id;
    int ground_count;
    int ground_items[CFG_AREA_ITEM_SLOTS];
    int slot;
    int i;

    if (game->mode == GAME_MODE_COMBAT) {
        render_inv_no_rummage_combat();
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
        render_inv_take_nothing();
        return 1;
    }
    if (game->bag_count + ground_count > game->bag_capacity) {
        render_inv_bag_full(game->bag_capacity);
        return 1;
    }
    for (i = 0; i < ground_count; ++i) {
        game_inv_bag_add(game, ground_items[i]);
    }
    for (slot = 0; slot < CFG_AREA_ITEM_SLOTS; ++slot) {
        game->room_item[room_id][slot] = ITEM_NONE;
    }
    for (i = 0; i < ground_count; ++i) {
        render_inv_pickup(item_name(ground_items[i]));
    }
    return 1;
}

int game_inv_cmd_drop(struct GameState *game, int item_arg)
{
    int room_id;
    int slot;

    if (game->mode == GAME_MODE_COMBAT) {
        render_inv_no_drop_combat();
        return 1;
    }
    room_id = game->player.room_id;
    if (!game_inv_player_has_item(game, item_arg)) {
        render_inv_not_carrying(item_name(item_arg));
        return 1;
    }
    slot = room_first_free_slot(game, room_id);
    if (slot < 0) {
        render_inv_ground_full(CFG_AREA_ITEM_SLOTS);
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
            render_inv_not_carrying(item_name(item_arg));
            return 1;
        }
    }
    game->room_item[room_id][slot] = item_arg;
    render_inv_drop(item_name(item_arg));
    return 1;
}

int game_inv_cmd_bag(struct GameState *game)
{
    render_inv_bag(game);
    return 1;
}

int game_inv_cmd_eat(struct GameState *game, int item_arg)
{
    if (game->mode == GAME_MODE_COMBAT) {
        render_inv_no_eat_combat();
        return 1;
    }
    if (game_inv_bag_find_index(game, item_arg) < 0) {
        render_inv_not_carrying(item_name(item_arg));
        return 1;
    }
    if (!item_is_edible(item_arg)) {
        render_inv_cannot_eat(item_name(item_arg));
        return 1;
    }
    game_inv_bag_remove_item(game, item_arg);
    if (!game_heal_player(game, item_food_heal_amount(item_arg))) {
        if (item_arg == ITEM_BERRY) {
            render_inv_eat_berry_full();
        } else {
            render_inv_eat_fish_full();
        }
    } else if (item_arg == ITEM_BERRY) {
        render_inv_eat_berry_healed(game->player_hp);
    } else {
        render_inv_eat_fish_healed(game->player_hp);
    }
    return 1;
}

int game_inv_cmd_use(struct GameState *game, int item_arg)
{
    if (game->mode == GAME_MODE_COMBAT) {
        render_inv_use_reply_combat();
        return 1;
    }
    if (game_inv_bag_find_index(game, item_arg) < 0) {
        render_inv_not_carrying(item_name(item_arg));
        return 1;
    }
    if (item_arg == ITEM_TORCH) {
        render_inv_use_torch();
        return 1;
    }
    if (item_arg == ITEM_SALVE) {
        game_inv_bag_remove_item(game, item_arg);
        if (game_heal_player(game, CFG_SALVE_HEAL_AMOUNT)) {
            render_inv_use_salve(game->player_hp);
        } else {
            render_inv_use_salve_full();
        }
        return 1;
    }
    if (item_arg == ITEM_SPEAR) {
        render_inv_use_spear();
        return 1;
    }
    render_inv_no_use(item_name(item_arg));
    return 1;
}

/* Remove one ingredient from bag if present, else from the wielded slot. */
static int craft_consume_one(struct GameState *game, int item_id)
{
    /* Crafting can consume a held weapon or a bag item; the source slot matters. */
    if (game_inv_bag_find_index(game, item_id) >= 0) {
        return game_inv_bag_remove_item(game, item_id);
    }
    if (game->weapon_equipped == item_id) {
        game->weapon_equipped = ITEM_NONE;
        return 1;
    }
    return 0;
}

int game_inv_cmd_craft(struct GameState *game, int item_arg)
{
    if (game->mode == GAME_MODE_COMBAT) {
        render_inv_no_craft_combat();
        return 1;
    }
    if (item_arg == ITEM_TORCH) {
        if (!game_inv_player_has_item(game, ITEM_STICK) ||
                !game_inv_player_has_item(game, ITEM_REED)) {
            render_inv_need_torch();
            return 1;
        }
        craft_consume_one(game, ITEM_STICK);
        craft_consume_one(game, ITEM_REED);
        game_inv_bag_add(game, ITEM_TORCH);
        render_inv_craft_torch();
        return 1;
    }
    if (item_arg == ITEM_SALVE) {
        if (!game_inv_player_has_item(game, ITEM_HERB) ||
                !game_inv_player_has_item(game, ITEM_BERRY)) {
            render_inv_need_salve();
            return 1;
        }
        craft_consume_one(game, ITEM_HERB);
        craft_consume_one(game, ITEM_BERRY);
        game_inv_bag_add(game, ITEM_SALVE);
        render_inv_craft_salve();
        return 1;
    }
    if (item_arg == ITEM_SPEAR) {
        if (!game_inv_player_has_item(game, ITEM_STICK) ||
                !game_inv_player_has_item(game, ITEM_STONE)) {
            render_inv_need_spear();
            return 1;
        }
        craft_consume_one(game, ITEM_STICK);
        craft_consume_one(game, ITEM_STONE);
        game_inv_bag_add(game, ITEM_SPEAR);
        render_inv_craft_spear();
        return 1;
    }
    render_inv_craft_unknown();
    return 1;
}

int game_inv_cmd_wield(struct GameState *game, int item_arg)
{
    int idx;
    int old_weapon;

    if (item_arg == game->weapon_equipped) {
        render_inv_already_wielding(item_name(item_arg));
        return 1;
    }
    idx = game_inv_bag_find_index(game, item_arg);
    if (idx < 0) {
        render_inv_not_carrying(item_name(item_arg));
        return 1;
    }
    if (!item_is_weapon(item_arg)) {
        render_inv_wield_not_weapon();
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
            render_inv_wield_stow_fail();
            return 1;
        }
    }
    game->weapon_equipped = item_arg;
    render_inv_wield(item_name(item_arg));
    return 1;
}

int game_inv_cmd_unwield(struct GameState *game)
{
    int room_id;
    int slot;
    int w;

    if (game->weapon_equipped == ITEM_NONE) {
        render_inv_unwield_empty();
        return 1;
    }
    w = game->weapon_equipped;
    if (game_inv_bag_add(game, w)) {
        game->weapon_equipped = ITEM_NONE;
        render_inv_unwield();
        return 1;
    }
    if (game->mode == GAME_MODE_COMBAT) {
        render_inv_unwield_cannot();
        return 1;
    }
    room_id = game->player.room_id;
    slot = room_first_free_slot(game, room_id);
    if (slot < 0) {
        render_inv_unwield_cannot();
        return 1;
    }
    game->weapon_equipped = ITEM_NONE;
    game->room_item[room_id][slot] = w;
    render_inv_unwield_ground(item_name(w));
    return 1;
}
