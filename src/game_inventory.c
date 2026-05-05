#include <stdio.h>
#include "game_inventory.h"
#include "game.h"
#include "items.h"

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
        printf("There is no body here to loot.\n");
        return 1;
    }
    ground_item = game->corpse_loot[room_id];
    if (ground_item == ITEM_NONE) {
        printf("The body has already been stripped clean.\n");
        return 1;
    }
    if (!game_inv_bag_add(game, ground_item)) {
        printf("Your bag is full. Drop something first.\n");
        return 1;
    }
    printf("You loot a %s from the body.\n", item_name(ground_item));
    game->corpse_loot[room_id] = ITEM_NONE;
    game->corpse_present[room_id] = 0;
    return 1;
}

int game_inv_cmd_take(struct GameState *game, int item_arg)
{
    int room_id;
    int ground_item;

    if (game->combat_active) {
        printf("You cannot rummage through gear mid-fight.\n");
        return 1;
    }
    room_id = game->player.room_id;
    ground_item = game->room_item[room_id];
    if (ground_item == ITEM_NONE) {
        printf("There is nothing here to take.\n");
        return 1;
    }
    if (item_arg != ground_item) {
        printf("You cannot take that from here.\n");
        return 1;
    }
    if (!game_inv_bag_add(game, ground_item)) {
        printf("Your bag is full (%d items max).\n", game->bag_capacity);
        return 1;
    }
    game->room_item[room_id] = ITEM_NONE;
    printf("You pick up the %s.\n", item_name(ground_item));
    return 1;
}

int game_inv_cmd_drop(struct GameState *game, int item_arg)
{
    int room_id;

    if (game->combat_active) {
        printf("Not while a blade is in your face.\n");
        return 1;
    }
    room_id = game->player.room_id;
    if (game_inv_bag_find_index(game, item_arg) < 0) {
        printf("You are not carrying a %s.\n", item_name(item_arg));
        return 1;
    }
    if (game->room_item[room_id] != ITEM_NONE) {
        printf("The ground here is already occupied by a %s.\n",
            item_name(game->room_item[room_id]));
        return 1;
    }
    game_inv_bag_remove_item(game, item_arg);
    game->room_item[room_id] = item_arg;
    printf("You drop the %s.\n", item_name(item_arg));
    return 1;
}

int game_inv_cmd_bag(struct GameState *game)
{
    int i;

    printf("Bag (%d/%d):", game->bag_count, game->bag_capacity);
    if (game->bag_count <= 0) {
        printf(" empty\n");
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
        printf("You cannot eat calmly during combat.\n");
        return 1;
    }
    if (game_inv_bag_find_index(game, item_arg) < 0) {
        printf("You are not carrying a %s.\n", item_name(item_arg));
        return 1;
    }
    if (!item_is_edible(item_arg)) {
        printf("You cannot eat the %s.\n", item_name(item_arg));
        return 1;
    }
    game_inv_bag_remove_item(game, item_arg);
    if (item_arg == ITEM_BERRY) {
        printf("You eat the berry. Tart, but fresh.\n");
    } else {
        printf("You eat the fish. Not ideal raw, but nourishing.\n");
    }
    return 1;
}

int game_inv_cmd_use(struct GameState *game, int item_arg)
{
    if (game->combat_active) {
        printf("In combat, use reply 1/2/3 for your turn.\n");
        return 1;
    }
    if (game_inv_bag_find_index(game, item_arg) < 0) {
        printf("You are not carrying a %s.\n", item_name(item_arg));
        return 1;
    }
    if (item_arg == ITEM_TORCH) {
        printf("You raise the torch; nearby details sharpen in warm light.\n");
        return 1;
    }
    if (item_arg == ITEM_SALVE) {
        game->player_hp += 5;
        if (game->player_hp > game->max_hp) game->player_hp = game->max_hp;
        printf("You apply the salve and recover 5 HP. HP now %d.\n",
            game->player_hp);
        game_inv_bag_remove_item(game, item_arg);
        return 1;
    }
    if (item_arg == ITEM_SPEAR) {
        printf("You test the spear's weight. Balanced enough.\n");
        return 1;
    }
    printf("You cannot find a practical use for the %s right now.\n",
        item_name(item_arg));
    return 1;
}

int game_inv_cmd_craft(struct GameState *game, int item_arg)
{
    if (game->combat_active) {
        printf("You cannot craft while fighting.\n");
        return 1;
    }
    if (item_arg == ITEM_TORCH) {
        if (game_inv_bag_find_index(game, ITEM_STICK) < 0 ||
                game_inv_bag_find_index(game, ITEM_REED) < 0) {
            printf("Craft torch needs: stick + reed.\n");
            return 1;
        }
        game_inv_bag_remove_item(game, ITEM_STICK);
        game_inv_bag_remove_item(game, ITEM_REED);
        game_inv_bag_add(game, ITEM_TORCH);
        printf("You bind a makeshift torch.\n");
        return 1;
    }
    if (item_arg == ITEM_SALVE) {
        if (game_inv_bag_find_index(game, ITEM_HERB) < 0 ||
                game_inv_bag_find_index(game, ITEM_BERRY) < 0) {
            printf("Craft salve needs: herb + berry.\n");
            return 1;
        }
        game_inv_bag_remove_item(game, ITEM_HERB);
        game_inv_bag_remove_item(game, ITEM_BERRY);
        game_inv_bag_add(game, ITEM_SALVE);
        printf("You mash a basic healing salve.\n");
        return 1;
    }
    if (item_arg == ITEM_SPEAR) {
        if (game_inv_bag_find_index(game, ITEM_STICK) < 0 ||
                game_inv_bag_find_index(game, ITEM_STONE) < 0) {
            printf("Craft spear needs: stick + stone.\n");
            return 1;
        }
        game_inv_bag_remove_item(game, ITEM_STICK);
        game_inv_bag_remove_item(game, ITEM_STONE);
        game_inv_bag_add(game, ITEM_SPEAR);
        printf("You lash a stone point to the stick and craft a spear.\n");
        return 1;
    }
    printf("You do not know how to craft that.\n");
    return 1;
}
