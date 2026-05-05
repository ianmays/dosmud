#ifndef GAME_INVENTORY_H
#define GAME_INVENTORY_H

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
