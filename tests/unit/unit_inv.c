#include "greatest.h"
#include "config.h"
#include "game.h"
#include "gout.h"
#include "invent.h"
#include "items.h"
#include "unit_util.h"

static int inv_loot(struct GameState *game, GameEventQueue *out)
{
    gout_reset(out);
    return game_inv_cmd_loot(game, out);
}

static int inv_take_all(struct GameState *game, GameEventQueue *out)
{
    gout_reset(out);
    return game_inv_cmd_take_all(game, out);
}

static int inv_take(struct GameState *game, int item_id, GameEventQueue *out)
{
    gout_reset(out);
    return game_inv_cmd_take(game, item_id, out);
}

static int inv_drop(struct GameState *game, int item_id, GameEventQueue *out)
{
    gout_reset(out);
    return game_inv_cmd_drop(game, item_id, out);
}

static int inv_bag(struct GameState *game, GameEventQueue *out)
{
    gout_reset(out);
    return game_inv_cmd_bag(game, out);
}

static int inv_eat(struct GameState *game, int item_id, GameEventQueue *out)
{
    gout_reset(out);
    return game_inv_cmd_eat(game, item_id, out);
}

static int inv_use(struct GameState *game, int item_id, GameEventQueue *out)
{
    gout_reset(out);
    return game_inv_cmd_use(game, item_id, out);
}

static int inv_craft(struct GameState *game, int item_id, GameEventQueue *out)
{
    gout_reset(out);
    return game_inv_cmd_craft(game, item_id, out);
}

static int inv_wield(struct GameState *game, int item_id, GameEventQueue *out)
{
    gout_reset(out);
    return game_inv_cmd_wield(game, item_id, out);
}

static int inv_unwield(struct GameState *game, GameEventQueue *out)
{
    gout_reset(out);
    return game_inv_cmd_unwield(game, out);
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
    game.corpse_loot[WORLD_ROOM_CAMP] = ITEM_BERRY;
    ASSERT_EQ(1, inv_loot(&game, &out));
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ITEM_ACTION_LOOT, out.events[0].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(ITEM_BERRY, out.events[0].arg2);
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_BERRY));
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
    game.corpse_loot[WORLD_ROOM_CAMP] = ITEM_BERRY;
    game.bag_count = game.bag_capacity;
    ASSERT_EQ(1, inv_loot(&game, &out));
    ASSERT_EQ(GAME_ITEM_OUTCOME_BAG_FULL_DROP, out.events[0].arg1);
    ASSERT_EQ(ITEM_BERRY, out.events[0].arg2);
    ASSERT_EQ(1, game.corpse_present[WORLD_ROOM_CAMP]);
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

SUITE(invent) {
    RUN_TEST(invent_ground_slots);
    RUN_TEST(invent_bag_add_remove);
    RUN_TEST(invent_take_drop_paths);
    RUN_TEST(invent_take_all_paths);
    RUN_TEST(invent_take_all_bag_full);
    RUN_TEST(invent_take_all_nothing);
    RUN_TEST(invent_take_combat_blocked);
    RUN_TEST(invent_eat_and_use);
    RUN_TEST(invent_bag_view_event);
    RUN_TEST(invent_eat_heals_damaged);
    RUN_TEST(invent_salve_at_max_hp);
    RUN_TEST(invent_craft_torch);
    RUN_TEST(invent_craft_missing_ingredients);
    RUN_TEST(invent_wield_and_unwield);
    RUN_TEST(invent_loot_corpse);
    RUN_TEST(invent_wield_already_and_not_weapon);
    RUN_TEST(invent_loot_bag_full);
    RUN_TEST(invent_craft_from_wielded_ingredient);
    RUN_TEST(invent_drop_not_carrying);
    RUN_TEST(invent_wield_swap_weapons);
    RUN_TEST(invent_unwield_to_ground);
}
