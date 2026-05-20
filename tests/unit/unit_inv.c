#include "greatest.h"
#include "config.h"
#include "game.h"
#include "invent.h"
#include "items.h"
#include "unit_util.h"

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

    unit_game_fresh(&game, 3u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.room_item[WORLD_ROOM_CAMP][0] = ITEM_STICK;
    ASSERT_EQ(1, game_inv_cmd_take(&game, ITEM_STICK));
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_STICK));
    ASSERT_EQ(1, game_inv_cmd_drop(&game, ITEM_STICK));
    ASSERT_EQ(0, game_inv_player_has_item(&game, ITEM_STICK));
    ASSERT_EQ(1, game_inv_cmd_take(&game, ITEM_REED));
    PASS();
}

TEST invent_take_combat_blocked(void)
{
    struct GameState game;

    unit_game_fresh(&game, 4u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.room_item[WORLD_ROOM_CAMP][0] = ITEM_STICK;
    game_set_mode_combat(&game);
    ASSERT_EQ(1, game_inv_cmd_take(&game, ITEM_STICK));
    ASSERT_EQ(0, game_inv_player_has_item(&game, ITEM_STICK));
    PASS();
}

TEST invent_eat_and_use(void)
{
    struct GameState game;

    unit_game_fresh(&game, 5u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_BERRY);
    game_inv_bag_add(&game, ITEM_STONE);
    ASSERT_EQ(1, game_inv_cmd_eat(&game, ITEM_BERRY));
    ASSERT_EQ(0, game_inv_player_has_item(&game, ITEM_BERRY));
    ASSERT_EQ(1, game_inv_cmd_eat(&game, ITEM_STONE));
    game_inv_bag_add(&game, ITEM_SALVE);
    game.player_hp = 5;
    ASSERT_EQ(1, game_inv_cmd_use(&game, ITEM_SALVE));
    ASSERT_EQ(10, game.player_hp);
    ASSERT_EQ(1, game_inv_cmd_use(&game, ITEM_SPEAR));
    PASS();
}

TEST invent_craft_torch(void)
{
    struct GameState game;

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_STICK);
    game_inv_bag_add(&game, ITEM_REED);
    ASSERT_EQ(1, game_inv_cmd_craft(&game, ITEM_TORCH));
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_TORCH));
    ASSERT_EQ(1, game_inv_cmd_craft(&game, ITEM_SALVE));
    PASS();
}

TEST invent_craft_missing_ingredients(void)
{
    struct GameState game;

    unit_game_fresh(&game, 7u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, game_inv_cmd_craft(&game, ITEM_TORCH));
    ASSERT_EQ(1, game_inv_cmd_craft(&game, ITEM_SPEAR));
    PASS();
}

TEST invent_wield_and_unwield(void)
{
    struct GameState game;

    unit_game_fresh(&game, 8u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_STICK);
    game_inv_bag_add(&game, ITEM_SPEAR);
    ASSERT_EQ(1, game_inv_cmd_wield(&game, ITEM_STICK));
    ASSERT_EQ(ITEM_STICK, game.weapon_equipped);
    ASSERT_EQ(1, game_inv_cmd_wield(&game, ITEM_SPEAR));
    ASSERT_EQ(ITEM_SPEAR, game.weapon_equipped);
    ASSERT_EQ(1, game_inv_cmd_unwield(&game));
    ASSERT_EQ(ITEM_NONE, game.weapon_equipped);
    PASS();
}

TEST invent_loot_corpse(void)
{
    struct GameState game;

    unit_game_fresh(&game, 9u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.corpse_present[WORLD_ROOM_CAMP] = 1;
    game.corpse_loot[WORLD_ROOM_CAMP] = ITEM_BERRY;
    ASSERT_EQ(1, game_inv_cmd_loot(&game));
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_BERRY));
    ASSERT_EQ(0, game.corpse_present[WORLD_ROOM_CAMP]);
    ASSERT_EQ(1, game_inv_cmd_loot(&game));
    PASS();
}

TEST invent_wield_already_and_not_weapon(void)
{
    struct GameState game;

    unit_game_fresh(&game, 10u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_STICK);
    game.weapon_equipped = ITEM_STICK;
    ASSERT_EQ(1, game_inv_cmd_wield(&game, ITEM_STICK));
    ASSERT_EQ(1, game_inv_cmd_wield(&game, ITEM_BERRY));
    PASS();
}

TEST invent_loot_bag_full(void)
{
    struct GameState game;

    unit_game_fresh(&game, 11u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.corpse_present[WORLD_ROOM_CAMP] = 1;
    game.corpse_loot[WORLD_ROOM_CAMP] = ITEM_BERRY;
    game.bag_count = game.bag_capacity;
    ASSERT_EQ(1, game_inv_cmd_loot(&game));
    ASSERT_EQ(1, game.corpse_present[WORLD_ROOM_CAMP]);
    PASS();
}

TEST invent_craft_from_wielded_ingredient(void)
{
    struct GameState game;

    unit_game_fresh(&game, 14u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.weapon_equipped = ITEM_STICK;
    game_inv_bag_add(&game, ITEM_REED);
    ASSERT_EQ(1, game_inv_cmd_craft(&game, ITEM_TORCH));
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_TORCH));
    PASS();
}

TEST invent_drop_not_carrying(void)
{
    struct GameState game;

    unit_game_fresh(&game, 15u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, game_inv_cmd_drop(&game, ITEM_BERRY));
    PASS();
}

TEST invent_wield_swap_weapons(void)
{
    struct GameState game;

    unit_game_fresh(&game, 13u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_STICK);
    game_inv_bag_add(&game, ITEM_SPEAR);
    ASSERT_EQ(1, game_inv_cmd_wield(&game, ITEM_STICK));
    ASSERT_EQ(1, game_inv_cmd_wield(&game, ITEM_SPEAR));
    ASSERT_EQ(ITEM_SPEAR, game.weapon_equipped);
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_STICK));
    PASS();
}

TEST invent_unwield_to_ground(void)
{
    struct GameState game;

    unit_game_fresh(&game, 12u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.weapon_equipped = ITEM_STICK;
    game.bag_count = game.bag_capacity;
    ASSERT_EQ(1, game_inv_cmd_unwield(&game));
    ASSERT_EQ(ITEM_NONE, game.weapon_equipped);
    PASS();
}

SUITE(invent) {
    RUN_TEST(invent_ground_slots);
    RUN_TEST(invent_bag_add_remove);
    RUN_TEST(invent_take_drop_paths);
    RUN_TEST(invent_take_combat_blocked);
    RUN_TEST(invent_eat_and_use);
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
