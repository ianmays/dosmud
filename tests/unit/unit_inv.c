#include "greatest.h"
#include "config.h"
#include "game.h"
#include "gatmos.h"
#include "gout.h"
#include "invent.h"
#include "items.h"
#include "unit_util.h"

static int inv_loot(struct GameState *game, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return game_inv_cmd_loot(game, 0, out);
}

/* Direct invent API; loot_all=1 mirrors CMD_LOOT_ALL from command_parse */
static int inv_loot_all(struct GameState *game, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return game_inv_cmd_loot(game, 1, out);
}

static int inv_take_all(struct GameState *game, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return game_inv_cmd_take_all(game, out);
}

static int inv_take(struct GameState *game, int item_id, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return game_inv_cmd_take(game, item_id, out);
}

static int inv_drop(struct GameState *game, int item_id, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return game_inv_cmd_drop(game, item_id, out);
}

static int inv_bag(struct GameState *game, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return game_inv_cmd_bag(game, out);
}

static int inv_eat(struct GameState *game, int item_id, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return game_inv_cmd_eat(game, item_id, out);
}

static int inv_use(struct GameState *game, int item_id, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return game_inv_cmd_use(game, item_id, out);
}

static int inv_craft(struct GameState *game, int item_id, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return game_inv_cmd_craft(game, item_id, out);
}

static int inv_wield(struct GameState *game, int item_id, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return game_inv_cmd_wield(game, item_id, out);
}

static int inv_unwield(struct GameState *game, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return game_inv_cmd_unwield(game, out);
}

static int inv_loot_reply(struct GameState *game, int choice, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return game_inv_cmd_loot_reply(game, choice, out);
}

static int room_has_item(const struct GameState *game, int room_id, int item_id)
{
    int slot;

    for (slot = 0; slot < CFG_AREA_ITEM_SLOTS; ++slot) {
        if (game->room_item[room_id][slot] == item_id) {
            return 1;
        }
    }
    return 0;
}

TEST invent_ground_slots(void)
{
    struct GameState game;
    int s;

    unit_game_fresh(&game, 1u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.room_item[WORLD_ROOM_CAMP][0] = ITEM_NONE;
    game.room_item[WORLD_ROOM_CAMP][1] = ITEM_NONE;
    game.room_item[WORLD_ROOM_CAMP][2] = ITEM_NONE;
    game.room_item[WORLD_ROOM_CAMP][3] = ITEM_NONE;
    ASSERT_EQ(1, game_room_ground_has_space(&game, WORLD_ROOM_CAMP));
    for (s = 0; s < CFG_AREA_ITEM_SLOTS; ++s) {
        ASSERT_EQ(1, game_room_ground_try_add(&game, WORLD_ROOM_CAMP, ITEM_STONE));
    }
    ASSERT_EQ(0, game_room_ground_has_space(&game, WORLD_ROOM_CAMP));
    ASSERT_EQ(0, game_room_ground_try_add(&game, WORLD_ROOM_CAMP, ITEM_BERRY));
    PASS();
}

TEST invent_bag_add_remove(void)
{
    struct GameState game;

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_STICK));
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_STICK));
    ASSERT_EQ(0, game_inv_bag_remove_item(&game, ITEM_FISH));
    ASSERT_EQ(1, game_inv_bag_remove_item(&game, ITEM_STICK));
    ASSERT_EQ(0, game_inv_player_has_item(&game, ITEM_STICK));
    PASS();
}

TEST invent_player_has_item_const_query(void)
{
    struct GameState game;
    const struct GameState *view;

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_STICK));
    view = &game;
    ASSERT_EQ(1, game_inv_player_has_item(view, ITEM_STICK));
    ASSERT_EQ(0, game_inv_player_has_item(view, ITEM_FISH));
    ASSERT_EQ(-1, game_inv_bag_find_index(view, ITEM_FISH));
    PASS();
}

TEST invent_remove_carried_prefers_weapon_then_bag(void)
{
    struct GameState game;

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_SPEAR));
    game.weapon_equipped = ITEM_SPEAR;
    ASSERT_EQ(1, game_inv_remove_carried_item(&game, ITEM_SPEAR));
    ASSERT_EQ(ITEM_NONE, game.weapon_equipped);
    ASSERT_EQ(0, game_inv_bag_find_index(&game, ITEM_SPEAR));
    ASSERT_EQ(1, game_inv_remove_carried_item(&game, ITEM_SPEAR));
    ASSERT_EQ(-1, game_inv_bag_find_index(&game, ITEM_SPEAR));
    PASS();
}

TEST invent_deliver_room_item_uses_bag_then_ground(void)
{
    struct GameState game;

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(GAME_ITEM_DELIVERY_BAG,
        game_inv_deliver_room_item(&game, WORLD_ROOM_CAMP, ITEM_SALVE));
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_SALVE));

    game.bag_count = game.bag_capacity;
    game.bag[0] = ITEM_STICK;
    game.bag[1] = ITEM_REED;
    game.bag[2] = ITEM_STONE;
    game.bag[3] = ITEM_BERRY;
    ASSERT_EQ(GAME_ITEM_DELIVERY_GROUND,
        game_inv_deliver_room_item(&game, WORLD_ROOM_CAMP, ITEM_HERB));
    ASSERT_EQ(1, room_has_item(&game, WORLD_ROOM_CAMP, ITEM_HERB));
    PASS();
}

TEST invent_take_drop_paths(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 3u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.room_item[WORLD_ROOM_CAMP][0] = ITEM_STICK;
    ASSERT_EQ(1, inv_take(&game, ITEM_STICK, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ITEM_ACTION_TAKE, out.events[0].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(ITEM_STICK, out.events[0].arg2);
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_STICK));
    ASSERT_EQ(1, inv_drop(&game, ITEM_STICK, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ITEM_ACTION_DROP, out.events[0].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(ITEM_STICK, out.events[0].arg2);
    ASSERT_EQ(0, game_inv_player_has_item(&game, ITEM_STICK));
    ASSERT_EQ(1, inv_take(&game, ITEM_REED, &out));
    ASSERT_EQ(GAME_ITEM_OUTCOME_NOT_HERE, out.events[0].arg1);
    PASS();
}

TEST invent_take_all_paths(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 18u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.room_item[WORLD_ROOM_CAMP][0] = ITEM_STICK;
    game.room_item[WORLD_ROOM_CAMP][1] = ITEM_REED;
    ASSERT_EQ(1, inv_take_all(&game, &out));
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ITEM_ACTION_TAKE, out.events[0].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(ITEM_STICK, out.events[0].arg2);
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[1].kind);
    ASSERT_EQ(GAME_ITEM_ACTION_TAKE, out.events[1].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[1].arg1);
    ASSERT_EQ(ITEM_REED, out.events[1].arg2);
    ASSERT_EQ(2, game.bag_count);
    ASSERT_EQ(ITEM_STICK, game.bag[0]);
    ASSERT_EQ(ITEM_REED, game.bag[1]);
    ASSERT_EQ(ITEM_NONE, game.room_item[WORLD_ROOM_CAMP][0]);
    ASSERT_EQ(ITEM_NONE, game.room_item[WORLD_ROOM_CAMP][1]);
    PASS();
}

TEST invent_take_all_bag_full(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 19u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.room_item[WORLD_ROOM_CAMP][0] = ITEM_STICK;
    game.room_item[WORLD_ROOM_CAMP][1] = ITEM_REED;
    game.bag_count = game.bag_capacity - 1;
    game.bag[0] = ITEM_BERRY;
    ASSERT_EQ(1, inv_take_all(&game, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ITEM_ACTION_TAKE, out.events[0].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_BAG_FULL, out.events[0].arg1);
    ASSERT_EQ(game.bag_capacity, out.events[0].arg3);
    ASSERT_EQ(game.bag_capacity - 1, game.bag_count);
    ASSERT_EQ(ITEM_STICK, game.room_item[WORLD_ROOM_CAMP][0]);
    ASSERT_EQ(ITEM_REED, game.room_item[WORLD_ROOM_CAMP][1]);
    PASS();
}

TEST invent_take_all_nothing(void)
{
    struct GameState game;
    GameEventQueue out;
    int slot;

    unit_game_fresh(&game, 20u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    for (slot = 0; slot < CFG_AREA_ITEM_SLOTS; ++slot) {
        game.room_item[WORLD_ROOM_CAMP][slot] = ITEM_NONE;
    }
    ASSERT_EQ(1, inv_take_all(&game, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_ITEM_OUTCOME_NOTHING_HERE, out.events[0].arg1);
    ASSERT_EQ(0, game.bag_count);
    PASS();
}

TEST invent_take_combat_blocked(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 4u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.room_item[WORLD_ROOM_CAMP][0] = ITEM_STICK;
    game_set_mode_combat(&game);
    ASSERT_EQ(1, inv_take(&game, ITEM_STICK, &out));
    ASSERT_EQ(GAME_ITEM_OUTCOME_BLOCKED_COMBAT, out.events[0].arg1);
    ASSERT_EQ(0, game_inv_player_has_item(&game, ITEM_STICK));
    ASSERT_EQ(1, inv_take_all(&game, &out));
    ASSERT_EQ(GAME_ITEM_OUTCOME_BLOCKED_COMBAT, out.events[0].arg1);
    ASSERT_EQ(0, game_inv_player_has_item(&game, ITEM_STICK));
    PASS();
}

TEST invent_eat_and_use(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 5u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_BERRY);
    game_inv_bag_add(&game, ITEM_STONE);
    ASSERT_EQ(1, inv_eat(&game, ITEM_BERRY, &out));
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ITEM_ACTION_EAT, out.events[0].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_HP_FULL, out.events[0].arg1);
    ASSERT_EQ(ITEM_BERRY, out.events[0].arg2);
    ASSERT_EQ(0, game_inv_player_has_item(&game, ITEM_BERRY));
    ASSERT_EQ(CFG_START_MAX_HP, game.player_hp);
    ASSERT_EQ(1, inv_eat(&game, ITEM_STONE, &out));
    ASSERT_EQ(GAME_ITEM_OUTCOME_WRONG_ITEM, out.events[0].arg1);
    ASSERT_EQ(ITEM_STONE, out.events[0].arg2);
    game_inv_bag_add(&game, ITEM_SALVE);
    game.player_hp = 5;
    ASSERT_EQ(1, inv_use(&game, ITEM_SALVE, &out));
    ASSERT_EQ(GAME_ITEM_ACTION_USE, out.events[0].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(ITEM_SALVE, out.events[0].arg2);
    ASSERT_EQ(10, out.events[0].arg3);
    ASSERT_EQ(10, game.player_hp);
    ASSERT_EQ(1, inv_use(&game, ITEM_SPEAR, &out));
    ASSERT_EQ(GAME_ITEM_OUTCOME_NOT_CARRYING, out.events[0].arg1);
    PASS();
}

TEST invent_eat_and_use_allowed_in_combat(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 51u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.player_hp = 5;
    ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_BERRY));
    ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_SALVE));
    game_set_mode_combat(&game);

    ASSERT_EQ(1, inv_eat(&game, ITEM_BERRY, &out));
    ASSERT_EQ(GAME_ITEM_ACTION_EAT, out.events[0].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(ITEM_BERRY, out.events[0].arg2);
    ASSERT_EQ(6, game.player_hp);

    ASSERT_EQ(1, inv_use(&game, ITEM_SALVE, &out));
    ASSERT_EQ(GAME_ITEM_ACTION_USE, out.events[0].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(ITEM_SALVE, out.events[0].arg2);
    ASSERT_EQ(11, game.player_hp);
    PASS();
}

TEST invent_drop_allowed_in_combat(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 52u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.weapon_equipped = ITEM_STICK;
    game_set_mode_combat(&game);

    ASSERT_EQ(1, inv_drop(&game, ITEM_STICK, &out));
    ASSERT_EQ(GAME_ITEM_ACTION_DROP, out.events[0].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(ITEM_STICK, out.events[0].arg2);
    ASSERT_EQ(ITEM_NONE, game.weapon_equipped);
    ASSERT_EQ(ITEM_STICK, game.room_item[WORLD_ROOM_CAMP][0]);
    PASS();
}

TEST invent_craft_allowed_in_combat(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 53u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_HERB);
    game_inv_bag_add(&game, ITEM_BERRY);
    game_set_mode_combat(&game);

    ASSERT_EQ(1, inv_craft(&game, ITEM_SALVE, &out));
    ASSERT_EQ(GAME_EVENT_CRAFT_RESULT, out.events[0].kind);
    ASSERT_EQ(ITEM_SALVE, out.events[0].arg0);
    ASSERT_EQ(GAME_CRAFT_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_SALVE));
    ASSERT_EQ(0, game_inv_player_has_item(&game, ITEM_HERB));
    ASSERT_EQ(0, game_inv_player_has_item(&game, ITEM_BERRY));
    PASS();
}

TEST invent_bag_view_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 21u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, inv_bag(&game, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_BAG_VIEW, out.events[0].kind);
    PASS();
}

TEST invent_eat_heals_damaged(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 16u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.player_hp = 5;
    game_inv_bag_add(&game, ITEM_BERRY);
    ASSERT_EQ(1, inv_eat(&game, ITEM_BERRY, &out));
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(6, out.events[0].arg3);
    ASSERT_EQ(6, game.player_hp);
    game_inv_bag_add(&game, ITEM_FISH);
    game.player_hp = 10;
    ASSERT_EQ(1, inv_eat(&game, ITEM_FISH, &out));
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(ITEM_FISH, out.events[0].arg2);
    ASSERT_EQ(12, out.events[0].arg3);
    ASSERT_EQ(12, game.player_hp);
    PASS();
}

TEST invent_salve_at_max_hp(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 17u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_SALVE);
    ASSERT_EQ(1, inv_use(&game, ITEM_SALVE, &out));
    ASSERT_EQ(GAME_ITEM_ACTION_USE, out.events[0].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_HP_FULL, out.events[0].arg1);
    ASSERT_EQ(ITEM_SALVE, out.events[0].arg2);
    ASSERT_EQ(CFG_START_MAX_HP, game.player_hp);
    ASSERT_EQ(-1, game_inv_bag_find_index(&game, ITEM_SALVE));
    PASS();
}

TEST invent_craft_torch(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_STICK);
    game_inv_bag_add(&game, ITEM_REED);
    ASSERT_EQ(1, inv_craft(&game, ITEM_TORCH, &out));
    ASSERT_EQ(GAME_EVENT_CRAFT_RESULT, out.events[0].kind);
    ASSERT_EQ(ITEM_TORCH, out.events[0].arg0);
    ASSERT_EQ(GAME_CRAFT_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_TORCH));
    ASSERT_EQ(1, inv_craft(&game, ITEM_SALVE, &out));
    ASSERT_EQ(GAME_CRAFT_OUTCOME_NEED_INGREDIENTS, out.events[0].arg1);
    PASS();
}

TEST invent_craft_torch_clears_night_lost(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 130u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.day_phase = GAME_NIGHT;
    game.night_lost = 1;
    game_inv_bag_add(&game, ITEM_STICK);
    game_inv_bag_add(&game, ITEM_REED);
    ASSERT_EQ(1, inv_craft(&game, ITEM_TORCH, &out));
    ASSERT_EQ(GAME_CRAFT_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(0, game.night_lost);
    ASSERT_EQ(0, gatmos_night_map_blanked(&game));
    PASS();
}

TEST invent_craft_missing_ingredients(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 7u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, inv_craft(&game, ITEM_TORCH, &out));
    ASSERT_EQ(GAME_CRAFT_OUTCOME_NEED_INGREDIENTS, out.events[0].arg1);
    ASSERT_EQ(ITEM_TORCH, out.events[0].arg0);
    ASSERT_EQ(1, inv_craft(&game, ITEM_SPEAR, &out));
    ASSERT_EQ(GAME_CRAFT_OUTCOME_NEED_INGREDIENTS, out.events[0].arg1);
    ASSERT_EQ(ITEM_SPEAR, out.events[0].arg0);
    PASS();
}

TEST invent_wield_and_unwield(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 8u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_STICK);
    game_inv_bag_add(&game, ITEM_SPEAR);
    ASSERT_EQ(1, inv_wield(&game, ITEM_STICK, &out));
    ASSERT_EQ(GAME_EVENT_EQUIP_RESULT, out.events[0].kind);
    ASSERT_EQ(ITEM_STICK, out.events[0].arg0);
    ASSERT_EQ(GAME_EQUIP_OUTCOME_WIELDED, out.events[0].arg1);
    ASSERT_EQ(ITEM_STICK, game.weapon_equipped);
    ASSERT_EQ(1, inv_wield(&game, ITEM_SPEAR, &out));
    ASSERT_EQ(ITEM_SPEAR, out.events[0].arg0);
    ASSERT_EQ(GAME_EQUIP_OUTCOME_WIELDED, out.events[0].arg1);
    ASSERT_EQ(ITEM_SPEAR, game.weapon_equipped);
    ASSERT_EQ(1, inv_unwield(&game, &out));
    ASSERT_EQ(ITEM_SPEAR, out.events[0].arg0);
    ASSERT_EQ(GAME_EQUIP_OUTCOME_UNWIELD_STOWED, out.events[0].arg1);
    ASSERT_EQ(ITEM_NONE, game.weapon_equipped);
    PASS();
}

TEST invent_loot_corpse(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 9u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.corpse_present[WORLD_ROOM_CAMP] = 1;
    game.corpse_item[WORLD_ROOM_CAMP][0] = ITEM_BERRY;
    game.corpse_item[WORLD_ROOM_CAMP][1] = ITEM_STICK;
    game.corpse_item[WORLD_ROOM_CAMP][2] = ITEM_NONE;
    ASSERT_EQ(1, inv_loot(&game, &out));
    ASSERT_EQ(GAME_EVENT_CORPSE_VIEW, out.events[0].kind);
    ASSERT_EQ(2, out.events[0].arg0);
    ASSERT_EQ(3, out.events[0].arg1);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_LOOT, game.dialogue);
    ASSERT_EQ(1, inv_loot_reply(&game, 1, &out));
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ITEM_ACTION_LOOT, out.events[0].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(ITEM_BERRY, out.events[0].arg2);
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_BERRY));
    ASSERT_EQ(1, game.corpse_present[WORLD_ROOM_CAMP]);
    ASSERT_EQ(ITEM_STICK, game.corpse_item[WORLD_ROOM_CAMP][0]);
    ASSERT_EQ(ITEM_NONE, game.corpse_item[WORLD_ROOM_CAMP][1]);
    ASSERT_EQ(1, inv_loot_reply(&game, 1, &out));
    ASSERT_EQ(0, game.corpse_present[WORLD_ROOM_CAMP]);
    ASSERT_EQ(1, inv_loot(&game, &out));
    ASSERT_EQ(GAME_ITEM_OUTCOME_NO_BODY, out.events[0].arg1);
    PASS();
}

TEST invent_wield_already_and_not_weapon(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 10u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_STICK);
    game.weapon_equipped = ITEM_STICK;
    ASSERT_EQ(1, inv_wield(&game, ITEM_STICK, &out));
    ASSERT_EQ(GAME_EQUIP_OUTCOME_ALREADY_WIELDING, out.events[0].arg1);
    ASSERT_EQ(1, inv_wield(&game, ITEM_BERRY, &out));
    ASSERT_EQ(GAME_EQUIP_OUTCOME_NOT_CARRYING, out.events[0].arg1);
    PASS();
}

TEST invent_loot_bag_full(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 11u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.corpse_present[WORLD_ROOM_CAMP] = 1;
    game.corpse_item[WORLD_ROOM_CAMP][0] = ITEM_BERRY;
    game.corpse_item[WORLD_ROOM_CAMP][1] = ITEM_NONE;
    game.corpse_item[WORLD_ROOM_CAMP][2] = ITEM_NONE;
    game.bag_count = game.bag_capacity;
    ASSERT_EQ(1, inv_loot(&game, &out));
    ASSERT_EQ(GAME_EVENT_CORPSE_VIEW, out.events[0].kind);
    ASSERT_EQ(1, inv_loot_reply(&game, 1, &out));
    ASSERT_EQ(GAME_ITEM_OUTCOME_BAG_FULL_DROP, out.events[0].arg1);
    ASSERT_EQ(ITEM_BERRY, out.events[0].arg2);
    ASSERT_EQ(GAME_EVENT_CORPSE_VIEW, out.events[1].kind);
    ASSERT_EQ(1, game.corpse_present[WORLD_ROOM_CAMP]);
    PASS();
}

TEST invent_loot_all_clears_corpse(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 23u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.corpse_present[WORLD_ROOM_CAMP] = 1;
    game.corpse_item[WORLD_ROOM_CAMP][0] = ITEM_BERRY;
    game.corpse_item[WORLD_ROOM_CAMP][1] = ITEM_STICK;
    game.corpse_item[WORLD_ROOM_CAMP][2] = ITEM_HERB;

    ASSERT_EQ(1, inv_loot_all(&game, &out));
    ASSERT_EQ(3, out.count);
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(ITEM_BERRY, out.events[0].arg2);
    ASSERT_EQ(ITEM_STICK, out.events[1].arg2);
    ASSERT_EQ(ITEM_HERB, out.events[2].arg2);
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(0, game.corpse_present[WORLD_ROOM_CAMP]);
    ASSERT_EQ(3, game.bag_count);
    PASS();
}

TEST invent_loot_all_stops_when_bag_fills(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 24u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.corpse_present[WORLD_ROOM_CAMP] = 1;
    game.corpse_item[WORLD_ROOM_CAMP][0] = ITEM_BERRY;
    game.corpse_item[WORLD_ROOM_CAMP][1] = ITEM_STICK;
    game.corpse_item[WORLD_ROOM_CAMP][2] = ITEM_HERB;
    game.bag_capacity = 2;

    ASSERT_EQ(1, inv_loot_all(&game, &out));
    ASSERT_EQ(4, out.count);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(ITEM_BERRY, out.events[0].arg2);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[1].arg1);
    ASSERT_EQ(ITEM_STICK, out.events[1].arg2);
    ASSERT_EQ(GAME_ITEM_OUTCOME_BAG_FULL_DROP, out.events[2].arg1);
    ASSERT_EQ(ITEM_HERB, out.events[2].arg2);
    ASSERT_EQ(GAME_EVENT_CORPSE_VIEW, out.events[3].kind);
    ASSERT_EQ(1, out.events[3].arg0);
    ASSERT_EQ(2, out.events[3].arg1);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_LOOT, game.dialogue);
    ASSERT_EQ(1, game.corpse_present[WORLD_ROOM_CAMP]);
    ASSERT_EQ(ITEM_HERB, game.corpse_item[WORLD_ROOM_CAMP][0]);
    PASS();
}

TEST invent_loot_invalid_choice_uses_visible_max(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 12u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.corpse_present[WORLD_ROOM_CAMP] = 1;
    game.corpse_item[WORLD_ROOM_CAMP][0] = ITEM_BERRY;
    game.corpse_item[WORLD_ROOM_CAMP][1] = ITEM_STICK;
    game.corpse_item[WORLD_ROOM_CAMP][2] = ITEM_HERB;
    ASSERT_EQ(1, inv_loot(&game, &out));
    ASSERT_EQ(1, inv_loot_reply(&game, 5, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_PICK_123, out.events[0].arg0);
    ASSERT_EQ(4, out.events[0].arg1);
    PASS();
}

TEST invent_loot_invalid_choice_single_item_uses_two_choice_max(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 13u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.corpse_present[WORLD_ROOM_CAMP] = 1;
    game.corpse_item[WORLD_ROOM_CAMP][0] = ITEM_BERRY;
    game.corpse_item[WORLD_ROOM_CAMP][1] = ITEM_NONE;
    game.corpse_item[WORLD_ROOM_CAMP][2] = ITEM_NONE;
    ASSERT_EQ(1, inv_loot(&game, &out));
    ASSERT_EQ(1, inv_loot_reply(&game, 3, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_PICK_123, out.events[0].arg0);
    ASSERT_EQ(2, out.events[0].arg1);
    PASS();
}

TEST invent_corpse_queue_view_uses_dense_live_slots(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 22u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.corpse_present[WORLD_ROOM_CAMP] = 1;
    game.corpse_item[WORLD_ROOM_CAMP][0] = ITEM_BERRY;
    game.corpse_item[WORLD_ROOM_CAMP][1] = ITEM_NONE;
    game.corpse_item[WORLD_ROOM_CAMP][2] = ITEM_HERB;

    game_event_queue_reset(&out);
    ASSERT_EQ(1, game_corpse_queue_view(&game, WORLD_ROOM_CAMP, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_CORPSE_VIEW, out.events[0].kind);
    ASSERT_EQ(WORLD_ROOM_CAMP, out.events[0].room_id);
    ASSERT_EQ(2, out.events[0].arg0);
    ASSERT_EQ(3, out.events[0].arg1);
    ASSERT_EQ(ITEM_BERRY, out.events[0].room_item[0]);
    ASSERT_EQ(ITEM_NONE, out.events[0].room_item[1]);
    ASSERT_EQ(ITEM_HERB, out.events[0].room_item[2]);
    PASS();
}

TEST invent_craft_from_wielded_ingredient(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 14u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.weapon_equipped = ITEM_STICK;
    game_inv_bag_add(&game, ITEM_REED);
    ASSERT_EQ(1, inv_craft(&game, ITEM_TORCH, &out));
    ASSERT_EQ(GAME_CRAFT_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_TORCH));
    PASS();
}

TEST invent_drop_not_carrying(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 15u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, inv_drop(&game, ITEM_BERRY, &out));
    ASSERT_EQ(GAME_ITEM_OUTCOME_NOT_CARRYING, out.events[0].arg1);
    PASS();
}

TEST invent_wield_swap_weapons(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 13u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_STICK);
    game_inv_bag_add(&game, ITEM_SPEAR);
    ASSERT_EQ(1, inv_wield(&game, ITEM_STICK, &out));
    ASSERT_EQ(1, inv_wield(&game, ITEM_SPEAR, &out));
    ASSERT_EQ(GAME_EQUIP_OUTCOME_WIELDED, out.events[0].arg1);
    ASSERT_EQ(ITEM_SPEAR, out.events[0].arg0);
    ASSERT_EQ(ITEM_SPEAR, game.weapon_equipped);
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_STICK));
    PASS();
}

TEST invent_unwield_to_ground(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 12u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.weapon_equipped = ITEM_STICK;
    game.bag_count = game.bag_capacity;
    ASSERT_EQ(1, inv_unwield(&game, &out));
    ASSERT_EQ(GAME_EQUIP_OUTCOME_UNWIELD_DROPPED, out.events[0].arg1);
    ASSERT_EQ(ITEM_STICK, out.events[0].arg0);
    ASSERT_EQ(ITEM_NONE, game.weapon_equipped);
    PASS();
}

TEST invent_player_corpse_transfer_is_stable_and_retains_root(void)
{
    struct GameState game;
    int transferred;
    int retained;
    int retained_item;
    int equipped;
    int replaced;

    unit_game_fresh(&game, 60u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ROAD, 0);
    game.bag_capacity = CFG_BAG_MAX;
    ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_BERRY));
    ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_MARSH_ROOT));
    ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_HERB));
    game.weapon_equipped = ITEM_SPEAR;
    game.player_corpse_present = 1;
    game.player_corpse_room = WORLD_ROOM_MARSH;
    game.player_corpse_item_count = 2;
    game.player_corpse_item[0] = ITEM_STONE;
    game.player_corpse_item[1] = ITEM_FISH;

    game_player_corpse_replace_from_inventory(&game, WORLD_ROOM_ROAD,
        &transferred, &retained, &retained_item, &equipped, &replaced);
    ASSERT_EQ(3, transferred);
    ASSERT_EQ(1, retained);
    ASSERT_EQ(ITEM_MARSH_ROOT, retained_item);
    ASSERT_EQ(ITEM_SPEAR, equipped);
    ASSERT_EQ(2, replaced);
    ASSERT_EQ(1, game.player_corpse_present);
    ASSERT_EQ(WORLD_ROOM_ROAD, game.player_corpse_room);
    ASSERT_EQ(ITEM_BERRY, game.player_corpse_item[0]);
    ASSERT_EQ(ITEM_HERB, game.player_corpse_item[1]);
    ASSERT_EQ(ITEM_SPEAR, game.player_corpse_item[2]);
    ASSERT_EQ(1, game.bag_count);
    ASSERT_EQ(ITEM_MARSH_ROOT, game.bag[0]);
    ASSERT_EQ(ITEM_NONE, game.weapon_equipped);
    PASS();
}

TEST invent_player_corpse_holds_full_bag_plus_weapon(void)
{
    struct GameState game;
    int transferred;
    int retained;
    int retained_item;
    int equipped;
    int replaced;
    int i;

    unit_game_fresh(&game, 61u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ROAD, 0);
    game.bag_capacity = CFG_BAG_MAX;
    for (i = 0; i < CFG_BAG_MAX; ++i) {
        ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_STONE));
    }
    game.weapon_equipped = ITEM_STICK;
    game_player_corpse_replace_from_inventory(&game, WORLD_ROOM_ROAD,
        &transferred, &retained, &retained_item, &equipped, &replaced);
    ASSERT_EQ(CFG_PLAYER_CORPSE_ITEM_SLOTS, transferred);
    ASSERT_EQ(ITEM_STICK,
        game.player_corpse_item[CFG_PLAYER_CORPSE_ITEM_SLOTS - 1]);
    ASSERT_EQ(0, game.bag_count);
    PASS();
}

TEST invent_player_corpse_pages_and_precedes_enemy_corpse(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 62u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ROAD, 0);
    game.bag_capacity = CFG_BAG_MAX;
    game.player_corpse_present = 1;
    game.player_corpse_room = WORLD_ROOM_ROAD;
    game.player_corpse_item_count = 5;
    game.player_corpse_item[0] = ITEM_STICK;
    game.player_corpse_item[1] = ITEM_HERB;
    game.player_corpse_item[2] = ITEM_FISH;
    game.player_corpse_item[3] = ITEM_SALVE;
    game.player_corpse_item[4] = ITEM_SPEAR;
    game.corpse_present[WORLD_ROOM_ROAD] = 1;
    game.corpse_item[WORLD_ROOM_ROAD][0] = ITEM_BERRY;

    ASSERT_EQ(1, game_player_corpse_is_in_room(&game, WORLD_ROOM_ROAD));
    ASSERT_EQ(1, inv_loot(&game, &out));
    ASSERT_EQ(GAME_CORPSE_KIND_PLAYER, out.events[0].arg2);
    ASSERT_EQ(3, out.events[0].arg0);
    ASSERT_EQ(1, inv_loot_reply(&game, 3, &out));
    ASSERT_EQ(ITEM_FISH, game.bag[0]);
    ASSERT_EQ(4, game.player_corpse_item_count);
    ASSERT_EQ(ITEM_SALVE, game.player_corpse_item[2]);
    ASSERT_EQ(1, inv_loot_all(&game, &out));
    ASSERT_EQ(0, game.player_corpse_present);
    ASSERT_EQ(1, game.corpse_present[WORLD_ROOM_ROAD]);

    ASSERT_EQ(1, inv_loot(&game, &out));
    ASSERT_EQ(GAME_CORPSE_KIND_ENEMY, out.events[0].arg2);
    ASSERT_EQ(ITEM_BERRY, out.events[0].room_item[0]);
    PASS();
}

SUITE(invent) {
    RUN_TEST(invent_ground_slots);
    RUN_TEST(invent_bag_add_remove);
    RUN_TEST(invent_player_has_item_const_query);
    RUN_TEST(invent_remove_carried_prefers_weapon_then_bag);
    RUN_TEST(invent_deliver_room_item_uses_bag_then_ground);
    RUN_TEST(invent_take_drop_paths);
    RUN_TEST(invent_take_all_paths);
    RUN_TEST(invent_take_all_bag_full);
    RUN_TEST(invent_take_all_nothing);
    RUN_TEST(invent_take_combat_blocked);
    RUN_TEST(invent_eat_and_use);
    RUN_TEST(invent_eat_and_use_allowed_in_combat);
    RUN_TEST(invent_drop_allowed_in_combat);
    RUN_TEST(invent_craft_allowed_in_combat);
    RUN_TEST(invent_bag_view_event);
    RUN_TEST(invent_eat_heals_damaged);
    RUN_TEST(invent_salve_at_max_hp);
    RUN_TEST(invent_craft_torch);
    RUN_TEST(invent_craft_torch_clears_night_lost);
    RUN_TEST(invent_craft_missing_ingredients);
    RUN_TEST(invent_wield_and_unwield);
    RUN_TEST(invent_loot_corpse);
    RUN_TEST(invent_wield_already_and_not_weapon);
    RUN_TEST(invent_loot_bag_full);
    RUN_TEST(invent_loot_all_clears_corpse);
    RUN_TEST(invent_loot_all_stops_when_bag_fills);
    RUN_TEST(invent_loot_invalid_choice_uses_visible_max);
    RUN_TEST(invent_loot_invalid_choice_single_item_uses_two_choice_max);
    RUN_TEST(invent_corpse_queue_view_uses_dense_live_slots);
    RUN_TEST(invent_craft_from_wielded_ingredient);
    RUN_TEST(invent_drop_not_carrying);
    RUN_TEST(invent_wield_swap_weapons);
    RUN_TEST(invent_unwield_to_ground);
    RUN_TEST(invent_player_corpse_transfer_is_stable_and_retains_root);
    RUN_TEST(invent_player_corpse_holds_full_bag_plus_weapon);
    RUN_TEST(invent_player_corpse_pages_and_precedes_enemy_corpse);
}
