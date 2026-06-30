/*
 * Inventory and ground-slot ownership live here; the module keeps bag, hand,
 * and room storage explicit so the DOS-sized buffers stay predictable.
 * #158: command handlers queue GAME_EVENT_* outcomes; grendr maps them to text.
 */

#include "config.h"
#include "invent.h"
#include "game.h"
#include "gout.h"
#include "items.h"

/*
 * #158: typed inventory events (payload layout in gout.h). Queue carries item
 * ids and numeric context only; grendr resolves item_name and player copy.
 */
static void push_item_result(GameEventQueue *out, int action, int outcome,
                             int item_id, int value)
{
    game_event_push(out, GAME_EVENT_ITEM_RESULT, action, outcome, item_id,
        value, 0);
}

/*
 * Snapshot corpse slots into a CORPSE_VIEW event; invent owns corpse_item[].
 * arg0/arg1 drive menu numbering in grendr; pad room_item[] to CFG_AREA_ITEM_SLOTS.
 */
static void push_corpse_view(GameEventQueue *out, struct GameState *game,
                             int room_id)
{
    int slot;
    int item_count;
    GameEvent *ev;

    item_count = 0;
    ev = game_event_push(out, GAME_EVENT_CORPSE_VIEW, 0, 0, 0, 0, 0);
    if (ev == 0) {
        return;
    }
    ev->room_id = room_id;
    for (slot = 0; slot < CFG_CORPSE_ITEM_SLOTS; ++slot) {
        ev->room_item[slot] = game->corpse_item[room_id][slot];
        if (game->corpse_item[room_id][slot] != ITEM_NONE) {
            item_count++;
        }
    }
    for (; slot < CFG_AREA_ITEM_SLOTS; ++slot) {
        ev->room_item[slot] = ITEM_NONE;
    }
    ev->arg0 = item_count;
    ev->arg1 = item_count + 1;
}

static void push_craft_result(GameEventQueue *out, int item_id, int outcome)
{
    game_event_push(out, GAME_EVENT_CRAFT_RESULT, item_id, outcome, 0, 0, 0);
}

static void push_equip_result(GameEventQueue *out, int item_id, int outcome)
{
    game_event_push(out, GAME_EVENT_EQUIP_RESULT, item_id, outcome, 0, 0, 0);
}

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

static int corpse_item_count(struct GameState *game, int room_id)
{
    int slot;
    int count;

    count = 0;
    for (slot = 0; slot < CFG_CORPSE_ITEM_SLOTS; ++slot) {
        if (game->corpse_item[room_id][slot] != ITEM_NONE) {
            count++;
        }
    }
    return count;
}

int game_corpse_queue_view(struct GameState *game, int room_id,
                           GameEventQueue *out)
{
    if (!game->corpse_present[room_id] || !game_corpse_has_loot(game, room_id)) {
        return 0;
    }
    push_corpse_view(out, game, room_id);
    return 1;
}

int game_corpse_has_loot(struct GameState *game, int room_id)
{
    return corpse_item_count(game, room_id) > 0;
}

int game_corpse_try_add(struct GameState *game, int room_id, int item_id)
{
    int slot;

    for (slot = 0; slot < CFG_CORPSE_ITEM_SLOTS; ++slot) {
        if (game->corpse_item[room_id][slot] == ITEM_NONE) {
            game->corpse_item[room_id][slot] = item_id;
            return 1;
        }
    }
    return 0;
}

void game_corpse_clear(struct GameState *game, int room_id)
{
    int slot;

    game->corpse_present[room_id] = 0;
    for (slot = 0; slot < CFG_CORPSE_ITEM_SLOTS; ++slot) {
        game->corpse_item[room_id][slot] = ITEM_NONE;
    }
}

static void corpse_remove_slot_compact(struct GameState *game, int room_id, int slot)
{
    int s;

    /* Compact after take so corpse slots stay dense and menu indices stay stable. */
    for (s = slot; s < CFG_CORPSE_ITEM_SLOTS - 1; ++s) {
        game->corpse_item[room_id][s] = game->corpse_item[room_id][s + 1];
    }
    game->corpse_item[room_id][CFG_CORPSE_ITEM_SLOTS - 1] = ITEM_NONE;
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

/*
 * Give/surrender consumes the wielded copy before a bag stack of the same item.
 */
int game_inv_remove_carried_item(struct GameState *game, int item_id)
{
    if (game->weapon_equipped == item_id) {
        game->weapon_equipped = ITEM_NONE;
        return 1;
    }
    return game_inv_bag_remove_item(game, item_id);
}

int game_inv_deliver_room_item(struct GameState *game, int room_id, int item_id)
{
    if (game_inv_bag_add(game, item_id)) {
        return GAME_ITEM_DELIVERY_BAG;
    }
    if (game_room_ground_try_add(game, room_id, item_id)) {
        return GAME_ITEM_DELIVERY_GROUND;
    }
    return GAME_ITEM_DELIVERY_NONE;
}

/*
 * Bulk loot stays invent-owned: drains corpse_item[] from slot 0 in visible
 * order; on bag full, re-queues CORPSE_VIEW like game_inv_cmd_loot_reply.
 */
static int loot_all_from_corpse(struct GameState *game, int room_id,
                                GameEventQueue *out)
{
    int item_id;

    while (game_corpse_has_loot(game, room_id)) {
        item_id = game->corpse_item[room_id][0];
        if (!game_inv_bag_add(game, item_id)) {
            push_item_result(out, GAME_ITEM_ACTION_LOOT,
                GAME_ITEM_OUTCOME_BAG_FULL_DROP, item_id, 0);
            push_corpse_view(out, game, room_id);
            return 1;
        }

        corpse_remove_slot_compact(game, room_id, 0);
        push_item_result(out, GAME_ITEM_ACTION_LOOT, GAME_ITEM_OUTCOME_OK,
            item_id, 0);
    }

    game_corpse_clear(game, room_id);
    game_set_mode_explore(game);
    return 1;
}

/*
 * loot_all=0 opens the interactive corpse menu (DIALOGUE_LOOT); replies go to
 * game_inv_cmd_loot_reply via CMD_REPLY in game.c. loot_all=1 bulk-transfers
 * without menu replies, from explore or while the loot menu is already open.
 */
int game_inv_cmd_loot(struct GameState *game, int loot_all, GameEventQueue *out)
{
    int room_id;

    room_id = game->player.room_id;
    if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_LOOT) {
        /* bare loot again means leave; loot all bulk-takes remaining slots */
        if (loot_all) {
            return loot_all_from_corpse(game, room_id, out);
        }
        push_item_result(out, GAME_ITEM_ACTION_LOOT,
            GAME_ITEM_OUTCOME_LEFT_BEHIND, ITEM_NONE, 0);
        game_set_mode_explore(game);
        return 1;
    }
    if (!game->corpse_present[room_id]) {
        push_item_result(out, GAME_ITEM_ACTION_LOOT, GAME_ITEM_OUTCOME_NO_BODY,
            ITEM_NONE, 0);
        return 1;
    }
    if (!game_corpse_has_loot(game, room_id)) {
        push_item_result(out, GAME_ITEM_ACTION_LOOT,
            GAME_ITEM_OUTCOME_BODY_STRIPPED, ITEM_NONE, 0);
        return 1;
    }
    if (loot_all) {
        /* DIALOGUE_LOOT before bulk take so bag-full can re-queue the menu */
        game_set_mode_dialogue(game, DIALOGUE_LOOT);
        return loot_all_from_corpse(game, room_id, out);
    }
    game_set_mode_dialogue(game, DIALOGUE_LOOT);
    game_corpse_queue_view(game, room_id, out);
    return 1;
}

/*
 * choice 1..item_count takes a corpse slot; item_count+1 leaves the body.
 * Stays in DIALOGUE_LOOT while items remain after a successful take.
 */
int game_inv_cmd_loot_reply(struct GameState *game, int choice, GameEventQueue *out)
{
    int room_id;
    int item_count;
    int item_id;

    if (game->mode != GAME_MODE_DIALOGUE || game->dialogue != DIALOGUE_LOOT) {
        return 0;
    }

    room_id = game->player.room_id;
    item_count = corpse_item_count(game, room_id);
    if (item_count <= 0) {
        game_set_mode_explore(game);
        push_item_result(out, GAME_ITEM_ACTION_LOOT,
            GAME_ITEM_OUTCOME_BODY_STRIPPED, ITEM_NONE, 0);
        return 1;
    }
    if (choice == item_count + 1) {
        push_item_result(out, GAME_ITEM_ACTION_LOOT,
            GAME_ITEM_OUTCOME_LEFT_BEHIND, ITEM_NONE, 0);
        game_set_mode_explore(game);
        return 1;
    }
    if (choice < 1 || choice > item_count) {
        game_event_push(out, GAME_EVENT_DIALOGUE_GUARD,
            GAME_DIALOGUE_GUARD_PICK_123, item_count + 1, 0, 0, 0);
        return 1;
    }

    item_id = game->corpse_item[room_id][choice - 1];
    if (!game_inv_bag_add(game, item_id)) {
        push_item_result(out, GAME_ITEM_ACTION_LOOT,
            GAME_ITEM_OUTCOME_BAG_FULL_DROP, item_id, 0);
        push_corpse_view(out, game, room_id);
        return 1;
    }

    corpse_remove_slot_compact(game, room_id, choice - 1);
    push_item_result(out, GAME_ITEM_ACTION_LOOT, GAME_ITEM_OUTCOME_OK, item_id,
        0);
    if (!game_corpse_has_loot(game, room_id)) {
        game_corpse_clear(game, room_id);
        game_set_mode_explore(game);
        return 1;
    }
    push_corpse_view(out, game, room_id);
    return 1;
}

int game_inv_cmd_take(struct GameState *game, int item_arg, GameEventQueue *out)
{
    int room_id;
    int ground_item;
    int slot;

    if (game->mode == GAME_MODE_COMBAT) {
        push_item_result(out, GAME_ITEM_ACTION_TAKE,
            GAME_ITEM_OUTCOME_BLOCKED_COMBAT, item_arg, 0);
        return 1;
    }
    room_id = game->player.room_id;
    if (room_ground_is_empty(game, room_id)) {
        push_item_result(out, GAME_ITEM_ACTION_TAKE,
            GAME_ITEM_OUTCOME_NOTHING_HERE, ITEM_NONE, 0);
        return 1;
    }
    slot = room_find_item_slot(game, room_id, item_arg);
    if (slot < 0) {
        push_item_result(out, GAME_ITEM_ACTION_TAKE,
            GAME_ITEM_OUTCOME_NOT_HERE, item_arg, 0);
        return 1;
    }
    ground_item = game->room_item[room_id][slot];
    if (!game_inv_bag_add(game, ground_item)) {
        push_item_result(out, GAME_ITEM_ACTION_TAKE,
            GAME_ITEM_OUTCOME_BAG_FULL, ground_item, game->bag_capacity);
        return 1;
    }
    room_remove_slot_compact(game, room_id, slot);
    push_item_result(out, GAME_ITEM_ACTION_TAKE, GAME_ITEM_OUTCOME_OK,
        ground_item, 0);
    return 1;
}

int game_inv_cmd_take_all(struct GameState *game, GameEventQueue *out)
{
    int room_id;
    int ground_count;
    int ground_items[CFG_AREA_ITEM_SLOTS];
    int slot;
    int i;

    if (game->mode == GAME_MODE_COMBAT) {
        push_item_result(out, GAME_ITEM_ACTION_TAKE,
            GAME_ITEM_OUTCOME_BLOCKED_COMBAT, ITEM_NONE, 0);
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
        push_item_result(out, GAME_ITEM_ACTION_TAKE,
            GAME_ITEM_OUTCOME_NOTHING_HERE, ITEM_NONE, 0);
        return 1;
    }
    if (game->bag_count + ground_count > game->bag_capacity) {
        push_item_result(out, GAME_ITEM_ACTION_TAKE,
            GAME_ITEM_OUTCOME_BAG_FULL, ITEM_NONE, game->bag_capacity);
        return 1;
    }
    for (i = 0; i < ground_count; ++i) {
        game_inv_bag_add(game, ground_items[i]);
    }
    for (slot = 0; slot < CFG_AREA_ITEM_SLOTS; ++slot) {
        game->room_item[room_id][slot] = ITEM_NONE;
    }
    for (i = 0; i < ground_count; ++i) {
        push_item_result(out, GAME_ITEM_ACTION_TAKE, GAME_ITEM_OUTCOME_OK,
            ground_items[i], 0);
    }
    return 1;
}

int game_inv_cmd_drop(struct GameState *game, int item_arg, GameEventQueue *out)
{
    int room_id;
    int slot;

    if (game->mode == GAME_MODE_COMBAT) {
        push_item_result(out, GAME_ITEM_ACTION_DROP,
            GAME_ITEM_OUTCOME_BLOCKED_COMBAT, item_arg, 0);
        return 1;
    }
    room_id = game->player.room_id;
    if (!game_inv_player_has_item(game, item_arg)) {
        push_item_result(out, GAME_ITEM_ACTION_DROP,
            GAME_ITEM_OUTCOME_NOT_CARRYING, item_arg, 0);
        return 1;
    }
    slot = room_first_free_slot(game, room_id);
    if (slot < 0) {
        push_item_result(out, GAME_ITEM_ACTION_DROP,
            GAME_ITEM_OUTCOME_GROUND_FULL, item_arg, CFG_AREA_ITEM_SLOTS);
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
            push_item_result(out, GAME_ITEM_ACTION_DROP,
                GAME_ITEM_OUTCOME_NOT_CARRYING, item_arg, 0);
            return 1;
        }
    }
    game->room_item[room_id][slot] = item_arg;
    push_item_result(out, GAME_ITEM_ACTION_DROP, GAME_ITEM_OUTCOME_OK,
        item_arg, 0);
    return 1;
}

int game_inv_cmd_bag(struct GameState *game, GameEventQueue *out)
{
    (void)game;
    game_event_push(out, GAME_EVENT_BAG_VIEW, 0, 0, 0, 0, 0);
    return 1;
}

int game_inv_cmd_eat(struct GameState *game, int item_arg, GameEventQueue *out)
{
    if (game->mode == GAME_MODE_COMBAT) {
        push_item_result(out, GAME_ITEM_ACTION_EAT,
            GAME_ITEM_OUTCOME_BLOCKED_COMBAT, item_arg, 0);
        return 1;
    }
    if (game_inv_bag_find_index(game, item_arg) < 0) {
        push_item_result(out, GAME_ITEM_ACTION_EAT,
            GAME_ITEM_OUTCOME_NOT_CARRYING, item_arg, 0);
        return 1;
    }
    if (!item_is_edible(item_arg)) {
        push_item_result(out, GAME_ITEM_ACTION_EAT,
            GAME_ITEM_OUTCOME_WRONG_ITEM, item_arg, 0);
        return 1;
    }
    game_inv_bag_remove_item(game, item_arg);
    if (!game_heal_player(game, item_food_heal_amount(item_arg))) {
        push_item_result(out, GAME_ITEM_ACTION_EAT, GAME_ITEM_OUTCOME_HP_FULL,
            item_arg, 0);
    } else {
        push_item_result(out, GAME_ITEM_ACTION_EAT, GAME_ITEM_OUTCOME_OK,
            item_arg, game->player_hp);
    }
    return 1;
}

int game_inv_cmd_use(struct GameState *game, int item_arg, GameEventQueue *out)
{
    if (game->mode == GAME_MODE_COMBAT) {
        push_item_result(out, GAME_ITEM_ACTION_USE,
            GAME_ITEM_OUTCOME_BLOCKED_COMBAT, item_arg, 0);
        return 1;
    }
    if (game_inv_bag_find_index(game, item_arg) < 0) {
        push_item_result(out, GAME_ITEM_ACTION_USE,
            GAME_ITEM_OUTCOME_NOT_CARRYING, item_arg, 0);
        return 1;
    }
    if (item_arg == ITEM_TORCH) {
        push_item_result(out, GAME_ITEM_ACTION_USE, GAME_ITEM_OUTCOME_OK,
            item_arg, 0);
        return 1;
    }
    if (item_arg == ITEM_SALVE) {
        game_inv_bag_remove_item(game, item_arg);
        if (game_heal_player(game, CFG_SALVE_HEAL_AMOUNT)) {
            push_item_result(out, GAME_ITEM_ACTION_USE, GAME_ITEM_OUTCOME_OK,
                item_arg, game->player_hp);
        } else {
            push_item_result(out, GAME_ITEM_ACTION_USE,
                GAME_ITEM_OUTCOME_HP_FULL, item_arg, 0);
        }
        return 1;
    }
    if (item_arg == ITEM_SPEAR) {
        push_item_result(out, GAME_ITEM_ACTION_USE, GAME_ITEM_OUTCOME_OK,
            item_arg, 0);
        return 1;
    }
    push_item_result(out, GAME_ITEM_ACTION_USE, GAME_ITEM_OUTCOME_WRONG_ITEM,
        item_arg, 0);
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

int game_inv_cmd_craft(struct GameState *game, int item_arg, GameEventQueue *out)
{
    if (game->mode == GAME_MODE_COMBAT) {
        push_craft_result(out, item_arg, GAME_CRAFT_OUTCOME_BLOCKED_COMBAT);
        return 1;
    }
    if (item_arg == ITEM_TORCH) {
        if (!game_inv_player_has_item(game, ITEM_STICK) ||
                !game_inv_player_has_item(game, ITEM_REED)) {
            push_craft_result(out, item_arg,
                GAME_CRAFT_OUTCOME_NEED_INGREDIENTS);
            return 1;
        }
        craft_consume_one(game, ITEM_STICK);
        craft_consume_one(game, ITEM_REED);
        game_inv_bag_add(game, ITEM_TORCH);
        push_craft_result(out, item_arg, GAME_CRAFT_OUTCOME_OK);
        return 1;
    }
    if (item_arg == ITEM_SALVE) {
        if (!game_inv_player_has_item(game, ITEM_HERB) ||
                !game_inv_player_has_item(game, ITEM_BERRY)) {
            push_craft_result(out, item_arg,
                GAME_CRAFT_OUTCOME_NEED_INGREDIENTS);
            return 1;
        }
        craft_consume_one(game, ITEM_HERB);
        craft_consume_one(game, ITEM_BERRY);
        game_inv_bag_add(game, ITEM_SALVE);
        push_craft_result(out, item_arg, GAME_CRAFT_OUTCOME_OK);
        return 1;
    }
    if (item_arg == ITEM_SPEAR) {
        if (!game_inv_player_has_item(game, ITEM_STICK) ||
                !game_inv_player_has_item(game, ITEM_STONE)) {
            push_craft_result(out, item_arg,
                GAME_CRAFT_OUTCOME_NEED_INGREDIENTS);
            return 1;
        }
        craft_consume_one(game, ITEM_STICK);
        craft_consume_one(game, ITEM_STONE);
        game_inv_bag_add(game, ITEM_SPEAR);
        push_craft_result(out, item_arg, GAME_CRAFT_OUTCOME_OK);
        return 1;
    }
    push_craft_result(out, item_arg, GAME_CRAFT_OUTCOME_UNKNOWN);
    return 1;
}

int game_inv_cmd_wield(struct GameState *game, int item_arg, GameEventQueue *out)
{
    int idx;
    int old_weapon;

    if (item_arg == game->weapon_equipped) {
        push_equip_result(out, item_arg,
            GAME_EQUIP_OUTCOME_ALREADY_WIELDING);
        return 1;
    }
    idx = game_inv_bag_find_index(game, item_arg);
    if (idx < 0) {
        push_equip_result(out, item_arg, GAME_EQUIP_OUTCOME_NOT_CARRYING);
        return 1;
    }
    if (!item_is_weapon(item_arg)) {
        push_equip_result(out, item_arg, GAME_EQUIP_OUTCOME_NOT_WEAPON);
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
            push_equip_result(out, item_arg, GAME_EQUIP_OUTCOME_STOW_FAIL);
            return 1;
        }
    }
    game->weapon_equipped = item_arg;
    push_equip_result(out, item_arg, GAME_EQUIP_OUTCOME_WIELDED);
    return 1;
}

int game_inv_cmd_unwield(struct GameState *game, GameEventQueue *out)
{
    int room_id;
    int slot;
    int w;

    if (game->weapon_equipped == ITEM_NONE) {
        push_equip_result(out, ITEM_NONE, GAME_EQUIP_OUTCOME_UNWIELD_EMPTY);
        return 1;
    }
    w = game->weapon_equipped;
    if (game_inv_bag_add(game, w)) {
        game->weapon_equipped = ITEM_NONE;
        push_equip_result(out, w, GAME_EQUIP_OUTCOME_UNWIELD_STOWED);
        return 1;
    }
    if (game->mode == GAME_MODE_COMBAT) {
        push_equip_result(out, w, GAME_EQUIP_OUTCOME_UNWIELD_CANNOT);
        return 1;
    }
    room_id = game->player.room_id;
    slot = room_first_free_slot(game, room_id);
    if (slot < 0) {
        push_equip_result(out, w, GAME_EQUIP_OUTCOME_UNWIELD_CANNOT);
        return 1;
    }
    game->weapon_equipped = ITEM_NONE;
    game->room_item[room_id][slot] = w;
    push_equip_result(out, w, GAME_EQUIP_OUTCOME_UNWIELD_DROPPED);
    return 1;
}
