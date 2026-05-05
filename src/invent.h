/* Inventory module API. Header is invent.h (not game_inventory.h) so the filename
 * stays within DOS 8.3 (8+3); "game_inventory.h" is too long for classic FAT. */

#ifndef INVENT_H
#define INVENT_H

struct GameState;

int game_inv_bag_find_index(struct GameState *game, int item_id);
int game_inv_bag_add(struct GameState *game, int item_id);
int game_inv_bag_remove_index(struct GameState *game, int index);
int game_inv_bag_remove_item(struct GameState *game, int item_id);

int game_inv_cmd_loot(struct GameState *game);
int game_inv_cmd_take(struct GameState *game, int item_arg);
int game_inv_cmd_drop(struct GameState *game, int item_arg);
int game_inv_cmd_bag(struct GameState *game);
int game_inv_cmd_eat(struct GameState *game, int item_arg);
int game_inv_cmd_use(struct GameState *game, int item_arg);
int game_inv_cmd_craft(struct GameState *game, int item_arg);

#endif
