/*
 * Inventory module API. The short basename keeps the header compatible with
 * FAT 8+3 DOS trees while exposing bag, ground, and craft ownership rules.
 */

#ifndef INVENT_H
#define INVENT_H

struct GameState;
struct GameEventQueue;

/* Result codes from game_inv_deliver_room_item bag-then-ground placement. */
#define GAME_ITEM_DELIVERY_NONE 0
#define GAME_ITEM_DELIVERY_BAG 1
#define GAME_ITEM_DELIVERY_GROUND 2

/* Places item_id on the ground if a slot is free; returns 1 if stored. */
int game_room_ground_try_add(struct GameState *game, int room_id, int item_id);
/* Returns 1 when at least one ground slot is empty in this room. */
int game_room_ground_has_space(struct GameState *game, int room_id);
/* Corpse loot slots (CFG_CORPSE_ITEM_SLOTS); owned by invent, mutated on defeat/loot. */
int game_corpse_try_add(struct GameState *game, int room_id, int item_id);
int game_corpse_has_loot(struct GameState *game, int room_id);
void game_corpse_clear(struct GameState *game, int room_id);
int game_corpse_queue_view(struct GameState *game, int room_id,
                           struct GameEventQueue *out);

int game_inv_bag_find_index(struct GameState *game, int item_id);
/* True when the item is in the bag or is the wielded weapon. */
int game_inv_player_has_item(struct GameState *game, int item_id);
int game_inv_bag_add(struct GameState *game, int item_id);
int game_inv_bag_remove_index(struct GameState *game, int index);
int game_inv_bag_remove_item(struct GameState *game, int item_id);
/* Remove one carried item, preferring the wielded slot before the bag. */
int game_inv_remove_carried_item(struct GameState *game, int item_id);
/* Bag first, then room ground. Returns GAME_ITEM_DELIVERY_* above. */
int game_inv_deliver_room_item(struct GameState *game, int room_id, int item_id);

/* loot_all bypasses the numbered menu and drains corpse slots until full or empty */
int game_inv_cmd_loot(struct GameState *game, int loot_all,
                      struct GameEventQueue *out);
/* CMD_REPLY handler while DIALOGUE_LOOT is active; choice is 1-based menu index. */
int game_inv_cmd_loot_reply(struct GameState *game, int choice,
                            struct GameEventQueue *out);
int game_inv_cmd_take_all(struct GameState *game, struct GameEventQueue *out);
int game_inv_cmd_take(struct GameState *game, int item_arg, struct GameEventQueue *out);
int game_inv_cmd_drop(struct GameState *game, int item_arg, struct GameEventQueue *out);
int game_inv_cmd_bag(struct GameState *game, struct GameEventQueue *out);
int game_inv_cmd_eat(struct GameState *game, int item_arg, struct GameEventQueue *out);
int game_inv_cmd_use(struct GameState *game, int item_arg, struct GameEventQueue *out);
int game_inv_cmd_craft(struct GameState *game, int item_arg, struct GameEventQueue *out);
int game_inv_cmd_wield(struct GameState *game, int item_arg, struct GameEventQueue *out);
int game_inv_cmd_unwield(struct GameState *game, struct GameEventQueue *out);

#endif
