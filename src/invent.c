/* Inventory implementation (DOS-friendly 8.3 name invent.c). */

#include <stdio.h>
#include "invent.h"
#include "game.h"
#include "items.h"
#include "txtres.h"

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
    if (index < 0 || index >= game->bag_count) {
        return ITEM_NONE;
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

    if (game->combat_active) {
        printf("%s", TXT_INV_NO_RUMMAGE_COMBAT);
        return 1;
    }
    room_id = game->player.room_id;
    ground_item = game->room_item[room_id];
    if (ground_item == ITEM_NONE) {
        printf("%s", TXT_INV_TAKE_NOTHING);
        return 1;
    }
    if (item_arg != ground_item) {
        printf("%s", TXT_INV_CANNOT_TAKE_HERE);
        return 1;
    }
    if (!game_inv_bag_add(game, ground_item)) {
        printf(TXT_INV_BAG_FULL_FMT, game->bag_capacity);
        return 1;
    }
    game->room_item[room_id] = ITEM_NONE;
    printf(TXT_INV_PICKUP_FMT, item_name(ground_item));
    return 1;
}

int game_inv_cmd_drop(struct GameState *game, int item_arg)
{
    int room_id;

    if (game->combat_active) {
        printf("%s", TXT_INV_NO_DROP_COMBAT);
        return 1;
    }
    room_id = game->player.room_id;
    if (game_inv_bag_find_index(game, item_arg) < 0) {
        printf(TXT_INV_NOT_CARRYING_FMT, item_name(item_arg));
        return 1;
    }
    if (game->room_item[room_id] != ITEM_NONE) {
        printf(TXT_INV_GROUND_OCCUPIED_FMT,
            item_name(game->room_item[room_id]));
        return 1;
    }
    game_inv_bag_remove_item(game, item_arg);
    game->room_item[room_id] = item_arg;
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
        game->player_hp += 5;
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
