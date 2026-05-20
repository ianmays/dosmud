#include "greatest.h"
#include "config.h"
#include "game.h"
#include "combat.h"
#include "invent.h"
#include "items.h"
#include "unit_util.h"

TEST combat_attack_bonus(void)
{
    struct GameState game;

    unit_game_fresh(&game, 1u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(CFG_START_DAMAGE_BONUS, combat_player_attack_bonus(&game));
    game.weapon_equipped = ITEM_SPEAR;
    ASSERT_EQ(CFG_START_DAMAGE_BONUS + CFG_WEAPON_SPEAR_DAMAGE_BONUS,
        combat_player_attack_bonus(&game));
    PASS();
}

TEST combat_start_mode(void)
{
    struct GameState game;
    int rolls[1];

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    rolls[0] = CFG_TEST_FIGHT_ENEMY_HP_SPREAD;
    game_roll_inject_begin(&game, rolls, 1);
    combat_start(&game);
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    ASSERT_EQ(CFG_COMBAT_ENEMY_HP_BASE + CFG_TEST_FIGHT_ENEMY_HP_SPREAD, game.combat.enemy_hp);
    PASS();
}

TEST combat_reply_defend_reduces_damage(void)
{
    struct GameState game;
    int rolls[1];

    unit_game_fresh(&game, 3u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 20;
    rolls[0] = CFG_TEST_COMBAT_DEFEND_ENEMY_DMG;
    game_roll_inject_begin(&game, rolls, 1);
    combat_resolve_reply(&game, 2);
    ASSERT_EQ(0, game.combat.defending);
    ASSERT_EQ(20, game.player_hp);
    PASS();
}

TEST combat_reply_salve_in_combat(void)
{
    struct GameState game;
    int rolls[1];

    unit_game_fresh(&game, 8u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 50;
    game.player_hp = 5;
    ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_SALVE));
    rolls[0] = 0;
    game_roll_inject_begin(&game, rolls, 1);
    combat_resolve_reply(&game, 3);
    ASSERT_EQ(9, game.player_hp);
    ASSERT_EQ(-1, game_inv_bag_find_index(&game, ITEM_SALVE));
    PASS();
}

TEST combat_victory_loot_and_xp(void)
{
    struct GameState game;
    int rolls[3];

    unit_game_fresh(&game, 4u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 1;
    rolls[0] = 0;
    rolls[1] = CFG_TEST_VICTORY_LOOT_STICK;
    rolls[2] = CFG_TEST_VICTORY_XP_SPREAD;
    game_roll_inject_begin(&game, rolls, 3);
    combat_resolve_reply(&game, 1);
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(1, game.corpse_present[WORLD_ROOM_CAMP]);
    ASSERT_EQ(ITEM_STICK, game.corpse_loot[WORLD_ROOM_CAMP]);
    ASSERT_EQ(CFG_COMBAT_KILL_XP_BASE + CFG_TEST_VICTORY_XP_SPREAD, game.xp);
    PASS();
}

TEST combat_invalid_choice(void)
{
    struct GameState game;

    unit_game_fresh(&game, 5u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 10;
    combat_resolve_reply(&game, 9);
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    PASS();
}

TEST combat_player_death(void)
{
    struct GameState game;
    int rolls[2];

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 50;
    game.player_hp = 1;
    rolls[0] = 0;
    rolls[1] = 99;
    game_roll_inject_begin(&game, rolls, 2);
    combat_resolve_reply(&game, 1);
    ASSERT_EQ(0, game.running);
    PASS();
}

TEST combat_loot_tiers(void)
{
    struct GameState game;
    int rolls[3];

    unit_game_fresh(&game, 10u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 1;
    rolls[0] = 0;
    rolls[1] = CFG_TEST_VICTORY_LOOT_SPEAR;
    rolls[2] = 0;
    game_roll_inject_begin(&game, rolls, 3);
    combat_resolve_reply(&game, 1);
    ASSERT_EQ(ITEM_SPEAR, game.corpse_loot[WORLD_ROOM_CAMP]);

    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 1;
    rolls[1] = CFG_TEST_VICTORY_LOOT_FISH;
    game_roll_inject_begin(&game, rolls, 3);
    combat_resolve_reply(&game, 1);
    ASSERT_EQ(ITEM_FISH, game.corpse_loot[WORLD_ROOM_CAMP]);
    PASS();
}

SUITE(combat) {
    RUN_TEST(combat_attack_bonus);
    RUN_TEST(combat_start_mode);
    RUN_TEST(combat_reply_defend_reduces_damage);
    RUN_TEST(combat_reply_salve_in_combat);
    RUN_TEST(combat_victory_loot_and_xp);
    RUN_TEST(combat_invalid_choice);
    RUN_TEST(combat_player_death);
    RUN_TEST(combat_loot_tiers);
}
