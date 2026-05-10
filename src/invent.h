/* Inventory module API. Header is invent.h (not game_inventory.h) so the filename
 * stays within DOS 8.3 (8+3); "game_inventory.h" is too long for classic FAT. */

#ifndef INVENT_H
#define INVENT_H

struct GameState;

/* Places item_id on the ground if a slot is free; returns 1 if stored. */
int game_room_ground_try_add(struct GameState *game, int room_id, int item_id);
/* Returns 1 when at least one ground slot is empty in this room. */
int game_room_ground_has_space(struct GameState *game, int room_id);

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
