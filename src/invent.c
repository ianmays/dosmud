/* Inventory implementation (DOS-friendly 8.3 name invent.c). */

#include <stdio.h>
#include "config.h"
#include "invent.h"
#include "game.h"
#include "items.h"
#include "txtres.h"

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
        printf("%s", TXT_INV_NO_BODY_LOOT);
        return 1;
    }
    ground_item = game->corpse_loot[room_id];
    if (ground_item == ITEM_NONE) {
        printf("%s", TXT_INV_BODY_STRIPPED);
        return 1;
    }
    if (!game_inv_bag_add(game, ground_item)) {
        printf("%s", TXT_INV_BAG_FULL_DROP);
        return 1;
    }
    printf(TXT_INV_LOOT_FMT, item_name(ground_item));
    game->corpse_loot[room_id] = ITEM_NONE;
    game->corpse_present[room_id] = 0;
    return 1;
}

int game_inv_cmd_take(struct GameState *game, int item_arg)
{
    int room_id;
    int ground_item;
    int slot;

    if (game->combat_active) {
        printf("%s", TXT_INV_NO_RUMMAGE_COMBAT);
        return 1;
    }
    room_id = game->player.room_id;
    if (room_ground_is_empty(game, room_id)) {
        printf("%s", TXT_INV_TAKE_NOTHING);
        return 1;
    }
    slot = room_find_item_slot(game, room_id, item_arg);
    if (slot < 0) {
        printf("%s", TXT_INV_CANNOT_TAKE_HERE);
        return 1;
    }
    ground_item = game->room_item[room_id][slot];
    if (!game_inv_bag_add(game, ground_item)) {
        printf(TXT_INV_BAG_FULL_FMT, game->bag_capacity);
        return 1;
    }
    room_remove_slot_compact(game, room_id, slot);
    printf(TXT_INV_PICKUP_FMT, item_name(ground_item));
    return 1;
}

int game_inv_cmd_drop(struct GameState *game, int item_arg)
{
    int room_id;
    int slot;

    if (game->combat_active) {
        printf("%s", TXT_INV_NO_DROP_COMBAT);
        return 1;
    }
    room_id = game->player.room_id;
    if (game_inv_bag_find_index(game, item_arg) < 0) {
        printf(TXT_INV_NOT_CARRYING_FMT, item_name(item_arg));
        return 1;
    }
    slot = room_first_free_slot(game, room_id);
    if (slot < 0) {
        printf(TXT_INV_GROUND_FULL_FMT, CFG_AREA_ITEM_SLOTS);
        return 1;
    }
    game_inv_bag_remove_item(game, item_arg);
    game->room_item[room_id][slot] = item_arg;
    printf(TXT_INV_DROP_FMT, item_name(item_arg));
    return 1;
}

int game_inv_cmd_bag(struct GameState *game)
{
    int i;

    printf(TXT_INV_BAG_HEADER_FMT, game->bag_count, game->bag_capacity);
    if (game->bag_count <= 0) {
        printf("%s", TXT_INV_BAG_EMPTY);
        return 1;
    }
    for (i = 0; i < game->bag_count; ++i) {
        printf(" %s", item_name(game->bag[i]));
        if (i < game->bag_count - 1) {
            printf(",");
        }
    }
    printf("\n");
    if (game->weapon_equipped != ITEM_NONE) {
        printf(TXT_INV_BAG_WIELDING_FMT, item_name(game->weapon_equipped));
    }
    return 1;
}

int game_inv_cmd_eat(struct GameState *game, int item_arg)
{
    if (game->combat_active) {
        printf("%s", TXT_INV_NO_EAT_COMBAT);
        return 1;
    }
    if (game_inv_bag_find_index(game, item_arg) < 0) {
        printf(TXT_INV_NOT_CARRYING_FMT, item_name(item_arg));
        return 1;
    }
    if (!item_is_edible(item_arg)) {
        printf(TXT_INV_CANNOT_EAT_FMT, item_name(item_arg));
        return 1;
    }
    game_inv_bag_remove_item(game, item_arg);
    if (item_arg == ITEM_BERRY) {
        printf("%s", TXT_INV_EAT_BERRY);
    } else {
        printf("%s", TXT_INV_EAT_FISH);
    }
    return 1;
}

int game_inv_cmd_use(struct GameState *game, int item_arg)
{
    if (game->combat_active) {
        printf("%s", TXT_INV_USE_REPLY_COMBAT);
        return 1;
    }
    if (game_inv_bag_find_index(game, item_arg) < 0) {
        printf(TXT_INV_NOT_CARRYING_FMT, item_name(item_arg));
        return 1;
    }
    if (item_arg == ITEM_TORCH) {
        printf("%s", TXT_INV_USE_TORCH);
        return 1;
    }
    if (item_arg == ITEM_SALVE) {
        game->player_hp += CFG_SALVE_HEAL_AMOUNT;
        if (game->player_hp > game->max_hp) game->player_hp = game->max_hp;
        printf(TXT_INV_USE_SALVE_FMT,
            game->player_hp);
        game_inv_bag_remove_item(game, item_arg);
        return 1;
    }
    if (item_arg == ITEM_SPEAR) {
        printf("%s", TXT_INV_USE_SPEAR);
        return 1;
    }
    printf(TXT_INV_NO_USE_FMT,
        item_name(item_arg));
    return 1;
}

int game_inv_cmd_craft(struct GameState *game, int item_arg)
{
    if (game->combat_active) {
        printf("%s", TXT_INV_NO_CRAFT_COMBAT);
        return 1;
    }
    if (item_arg == ITEM_TORCH) {
        if (game_inv_bag_find_index(game, ITEM_STICK) < 0 ||
                game_inv_bag_find_index(game, ITEM_REED) < 0) {
            printf("%s", TXT_INV_NEED_TORCH);
            return 1;
        }
        game_inv_bag_remove_item(game, ITEM_STICK);
        game_inv_bag_remove_item(game, ITEM_REED);
        game_inv_bag_add(game, ITEM_TORCH);
        printf("%s", TXT_INV_CRAFT_TORCH);
        return 1;
    }
    if (item_arg == ITEM_SALVE) {
        if (game_inv_bag_find_index(game, ITEM_HERB) < 0 ||
                game_inv_bag_find_index(game, ITEM_BERRY) < 0) {
            printf("%s", TXT_INV_NEED_SALVE);
            return 1;
        }
        game_inv_bag_remove_item(game, ITEM_HERB);
        game_inv_bag_remove_item(game, ITEM_BERRY);
        game_inv_bag_add(game, ITEM_SALVE);
        printf("%s", TXT_INV_CRAFT_SALVE);
        return 1;
    }
    if (item_arg == ITEM_SPEAR) {
        if (game_inv_bag_find_index(game, ITEM_STICK) < 0 ||
                game_inv_bag_find_index(game, ITEM_STONE) < 0) {
            printf("%s", TXT_INV_NEED_SPEAR);
            return 1;
        }
        game_inv_bag_remove_item(game, ITEM_STICK);
        game_inv_bag_remove_item(game, ITEM_STONE);
        game_inv_bag_add(game, ITEM_SPEAR);
        printf("%s", TXT_INV_CRAFT_SPEAR);
        return 1;
    }
    printf("%s", TXT_INV_CRAFT_UNKNOWN);
    return 1;
}

int game_inv_cmd_wield(struct GameState *game, int item_arg)
{
    if (game_inv_bag_find_index(game, item_arg) < 0) {
        printf(TXT_INV_NOT_CARRYING_FMT, item_name(item_arg));
        return 1;
    }
    if (!item_is_weapon(item_arg)) {
        printf("%s", TXT_INV_WIELD_NOT_WEAPON);
        return 1;
    }
    game->weapon_equipped = item_arg;
    printf(TXT_INV_WIELD_FMT, item_name(item_arg));
    return 1;
}

int game_inv_cmd_unwield(struct GameState *game)
{
    if (game->weapon_equipped == ITEM_NONE) {
        printf("%s", TXT_INV_UNWIELD_EMPTY);
        return 1;
    }
    game->weapon_equipped = ITEM_NONE;
    printf("%s", TXT_INV_UNWIELD);
    return 1;
}
