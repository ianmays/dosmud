#include "greatest.h"
#include "config.h"
#include "game.h"
#include "combat.h"
#include "gout.h"
#include "invent.h"
#include "items.h"
#include "npc.h"
#include "unit_util.h"

/* Wraps combat_start with queue reset; initiator selects the opening strike path. */
static void start_combat_out(struct GameState *game, GameEventQueue *out,
                             int initiator)
{
    game_event_queue_reset(out);
    combat_start(game, out, initiator);
}

static void resolve_reply_out(struct GameState *game, int choice,
                              GameEventQueue *out)
{
    game_event_queue_reset(out);
    combat_resolve_reply(game, choice, out);
}

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
    GameEventQueue out;
    int rolls[2];

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    rolls[0] = CFG_TEST_FIGHT_ENEMY_HP_SPREAD;
    rolls[1] = 0;
    game_roll_inject_begin(&game, rolls, 2);
    start_combat_out(&game, &out, COMBAT_INITIATOR_PLAYER);
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    ASSERT_EQ(CFG_COMBAT_ENEMY_HP_BASE + CFG_TEST_FIGHT_ENEMY_HP_SPREAD -
        CFG_COMBAT_PLAYER_HIT_BASE - CFG_START_DAMAGE_BONUS,
        game.combat.enemy_hp);
    ASSERT_EQ(4, out.count);
    ASSERT_EQ(GAME_EVENT_COMBAT, out.events[0].kind);
    ASSERT_EQ(GAME_COMBAT_PHASE_START, out.events[0].arg0);
    ASSERT_EQ(game.player_hp, out.events[0].arg1);
    ASSERT_EQ(CFG_COMBAT_ENEMY_HP_BASE + CFG_TEST_FIGHT_ENEMY_HP_SPREAD,
        out.events[0].arg2);
    ASSERT_EQ(1, out.events[0].arg3);
    ASSERT_EQ(GAME_COMBAT_PHASE_PLAYER_DAMAGE, out.events[1].arg0);
    ASSERT_EQ(CFG_COMBAT_PLAYER_HIT_BASE + CFG_START_DAMAGE_BONUS,
        out.events[1].arg1);
    ASSERT_EQ(GAME_COMBAT_PHASE_STATUS, out.events[2].arg0);
    ASSERT_EQ(game.player_hp, out.events[2].arg1);
    ASSERT_EQ(game.combat.enemy_hp, out.events[2].arg2);
    ASSERT_EQ(GAME_COMBAT_PHASE_MENU, out.events[3].arg0);
    PASS();
}

TEST combat_start_uses_enemy_level_scaling(void)
{
    /* Direct combat_start: level comes from DIALOGUE_ENEMY slot, not combat_enemy_level default. */
    struct GameState game;
    GameEventQueue out;
    int rolls[2];
    int slot;

    unit_game_fresh(&game, 15u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    slot = npc_find_by_actor(&game, GAME_DIALOGUE_ACTOR_BANDIT);
    ASSERT(slot >= 0);
    game.npcs[slot].dialogue = DIALOGUE_ENEMY;
    game.npcs[slot].level = 3;
    rolls[0] = CFG_TEST_FIGHT_ENEMY_HP_SPREAD;
    rolls[1] = 0;
    game_roll_inject_begin(&game, rolls, 2);
    start_combat_out(&game, &out, COMBAT_INITIATOR_PLAYER);
    ASSERT_EQ(3, game.combat.enemy_level);
    ASSERT_EQ(CFG_COMBAT_ENEMY_HP_BASE +
            (2 * CFG_COMBAT_ENEMY_HP_PER_LEVEL) +
            CFG_TEST_FIGHT_ENEMY_HP_SPREAD -
            CFG_COMBAT_PLAYER_HIT_BASE - CFG_START_DAMAGE_BONUS,
        game.combat.enemy_hp);
    ASSERT_EQ(3, out.events[0].arg3);
    PASS();
}

TEST combat_replay_menu_emits_menu_phase(void)
{
    GameEventQueue out;

    game_event_queue_reset(&out);
    combat_replay_menu(&out);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_COMBAT, out.events[0].kind);
    ASSERT_EQ(GAME_COMBAT_PHASE_MENU, out.events[0].arg0);
    PASS();
}

TEST combat_enemy_initiative_strikes_before_menu(void)
{
    /* Direct combat_start: roll0 enemy HP spread, roll1 enemy damage spread. */
    struct GameState game;
    GameEventQueue out;
    int rolls[2];

    unit_game_fresh(&game, 18u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    rolls[0] = CFG_TEST_FIGHT_ENEMY_HP_SPREAD;
    rolls[1] = 0;
    game_roll_inject_begin(&game, rolls, 2);
    start_combat_out(&game, &out, COMBAT_INITIATOR_ENEMY);
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    ASSERT_EQ(CFG_START_MAX_HP - CFG_COMBAT_ENEMY_DMG_BASE, game.player_hp);
    ASSERT_EQ(4, out.count);
    ASSERT_EQ(GAME_COMBAT_PHASE_START, out.events[0].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_ENEMY_DAMAGE, out.events[1].arg0);
    ASSERT_EQ(CFG_COMBAT_ENEMY_DMG_BASE, out.events[1].arg1);
    ASSERT_EQ(GAME_COMBAT_PHASE_STATUS, out.events[2].arg0);
    ASSERT_EQ(game.player_hp, out.events[2].arg1);
    ASSERT_EQ(game.combat.enemy_hp, out.events[2].arg2);
    ASSERT_EQ(GAME_COMBAT_PHASE_MENU, out.events[3].arg0);
    PASS();
}

TEST combat_reply_defend_reduces_damage(void)
{
    struct GameState game;
    GameEventQueue out;
    int rolls[1];

    unit_game_fresh(&game, 3u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 20;
    game.combat.enemy_level = 1;
    rolls[0] = CFG_TEST_COMBAT_DEFEND_ENEMY_DMG;
    game_roll_inject_begin(&game, rolls, 1);
    resolve_reply_out(&game, 2, &out);
    ASSERT_EQ(0, game.combat.defending);
    ASSERT_EQ(20, game.player_hp);
    ASSERT_EQ(4, out.count);
    ASSERT_EQ(GAME_EVENT_COMBAT, out.events[0].kind);
    ASSERT_EQ(GAME_COMBAT_PHASE_BRACED, out.events[0].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_ENEMY_DAMAGE, out.events[1].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_STATUS, out.events[2].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_MENU, out.events[3].arg0);
    PASS();
}

TEST combat_reply_salve_in_combat(void)
{
    struct GameState game;
    GameEventQueue out;
    int rolls[1];

    unit_game_fresh(&game, 8u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 50;
    game.combat.enemy_level = 1;
    game.player_hp = 5;
    ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_SALVE));
    rolls[0] = 0;
    game_roll_inject_begin(&game, rolls, 1);
    resolve_reply_out(&game, 3, &out);
    ASSERT_EQ(9, game.player_hp);
    ASSERT_EQ(-1, game_inv_bag_find_index(&game, ITEM_SALVE));
    ASSERT_EQ(4, out.count);
    ASSERT_EQ(GAME_EVENT_COMBAT, out.events[0].kind);
    ASSERT_EQ(GAME_COMBAT_PHASE_SALVE_HEAL, out.events[0].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_ENEMY_DAMAGE, out.events[1].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_STATUS, out.events[2].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_MENU, out.events[3].arg0);
    PASS();
}

TEST combat_reply_salve_at_full_hp(void)
{
    struct GameState game;
    GameEventQueue out;
    int rolls[1];

    unit_game_fresh(&game, 11u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 50;
    game.combat.enemy_level = 1;
    game.player_hp = CFG_START_MAX_HP;
    ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_SALVE));
    rolls[0] = CFG_TEST_COMBAT_SALVE_ENEMY_DMG;
    game_roll_inject_begin(&game, rolls, 1);
    resolve_reply_out(&game, 3, &out);
    ASSERT_EQ(CFG_START_MAX_HP - 1, game.player_hp);
    ASSERT_EQ(-1, game_inv_bag_find_index(&game, ITEM_SALVE));
    ASSERT_EQ(4, out.count);
    ASSERT_EQ(GAME_EVENT_COMBAT, out.events[0].kind);
    ASSERT_EQ(GAME_COMBAT_PHASE_SALVE_FULL, out.events[0].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_ENEMY_DAMAGE, out.events[1].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_STATUS, out.events[2].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_MENU, out.events[3].arg0);
    PASS();
}

TEST combat_victory_loot_and_xp(void)
{
    struct GameState game;
    GameEventQueue out;
    int rolls[4];

    unit_game_fresh(&game, 4u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 1;
    game.combat.enemy_level = 1;
    rolls[0] = 0;
    rolls[1] = CFG_TEST_VICTORY_LOOT_COUNT_ONE;
    rolls[2] = CFG_TEST_VICTORY_LOOT_STICK;
    rolls[3] = CFG_TEST_VICTORY_XP_SPREAD;
    game_roll_inject_begin(&game, rolls, 4);
    resolve_reply_out(&game, 1, &out);
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(-1, npc_find_by_dialogue(&game, DIALOGUE_ENEMY));
    ASSERT_EQ(1, game.corpse_present[WORLD_ROOM_CAMP]);
    ASSERT_EQ(ITEM_STICK, game.corpse_item[WORLD_ROOM_CAMP][0]);
    ASSERT_EQ(CFG_COMBAT_KILL_XP_BASE + CFG_TEST_VICTORY_XP_SPREAD, game.xp);
    ASSERT_EQ(3, out.count);
    ASSERT_EQ(GAME_COMBAT_PHASE_PLAYER_DAMAGE, out.events[0].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_ENEMY_DEFEATED, out.events[1].arg0);
    ASSERT_EQ(1, out.events[1].arg3);
    ASSERT_EQ(GAME_EVENT_XP_GAIN, out.events[2].kind);
    PASS();
}

TEST combat_victory_xp_scales_with_enemy_level(void)
{
    struct GameState game;
    GameEventQueue out;
    int rolls[4];

    unit_game_fresh(&game, 16u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 1;
    game.combat.enemy_level = 3;
    rolls[0] = 0;
    rolls[1] = CFG_TEST_VICTORY_LOOT_COUNT_ONE;
    rolls[2] = CFG_TEST_VICTORY_LOOT_STICK;
    rolls[3] = CFG_TEST_VICTORY_XP_SPREAD;
    game_roll_inject_begin(&game, rolls, 4);
    resolve_reply_out(&game, 1, &out);
    ASSERT_EQ(2, game.level);
    ASSERT_EQ(4, out.count);
    ASSERT_EQ(GAME_EVENT_XP_GAIN, out.events[2].kind);
    ASSERT_EQ(CFG_COMBAT_KILL_XP_BASE +
            (2 * CFG_COMBAT_KILL_XP_PER_LEVEL) +
            CFG_TEST_VICTORY_XP_SPREAD,
        out.events[2].arg0);
    ASSERT_EQ(GAME_EVENT_STAT_CHANGE, out.events[3].kind);
    PASS();
}

TEST combat_victory_clears_active_bandit_slot(void)
{
    struct GameState game;
    GameEventQueue out;
    int rolls[4];

    unit_game_fresh(&game, 12u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT(npc_begin_encounter(&game, GAME_DIALOGUE_ACTOR_BANDIT,
        DIALOGUE_ENEMY, GAME_ENCOUNTER_BANDIT, WORLD_ROOM_CAMP, 0, &out) >= 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 1;
    game.combat.enemy_level = 1;
    rolls[0] = 0;
    rolls[1] = CFG_TEST_VICTORY_LOOT_COUNT_ONE;
    rolls[2] = CFG_TEST_VICTORY_LOOT_STICK;
    rolls[3] = 0;
    game_roll_inject_begin(&game, rolls, 4);
    resolve_reply_out(&game, 1, &out);
    ASSERT_EQ(-1, npc_find_by_dialogue(&game, DIALOGUE_ENEMY));
    ASSERT_EQ(0, npc_is_present(&game, GAME_DIALOGUE_ACTOR_BANDIT,
        WORLD_ROOM_CAMP));
    PASS();
}

TEST combat_invalid_choice(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 5u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 10;
    game.combat.enemy_level = 1;
    resolve_reply_out(&game, 9, &out);
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_COMBAT_PHASE_INVALID_CHOICE, out.events[0].arg0);
    PASS();
}

TEST combat_player_death(void)
{
    struct GameState game;
    GameEventQueue out;
    int rolls[2];

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 50;
    game.combat.enemy_level = 1;
    game.player_hp = 1;
    rolls[0] = 0;
    rolls[1] = 99;
    game_roll_inject_begin(&game, rolls, 2);
    resolve_reply_out(&game, 1, &out);
    ASSERT_EQ(0, game.running);
    ASSERT_EQ(GAME_EVENT_COMBAT, out.events[out.count - 1].kind);
    ASSERT_EQ(GAME_COMBAT_PHASE_PLAYER_DOWN, out.events[out.count - 1].arg0);
    PASS();
}

TEST combat_loot_tiers(void)
{
    struct GameState game;
    GameEventQueue out;
    int rolls[4];

    unit_game_fresh(&game, 10u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 1;
    game.combat.enemy_level = 1;
    rolls[0] = 0;
    rolls[1] = CFG_TEST_VICTORY_LOOT_COUNT_ONE;
    rolls[2] = CFG_TEST_VICTORY_LOOT_SPEAR;
    rolls[3] = 0;
    game_roll_inject_begin(&game, rolls, 4);
    resolve_reply_out(&game, 1, &out);
    ASSERT_EQ(ITEM_SPEAR, game.corpse_item[WORLD_ROOM_CAMP][0]);

    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 1;
    game.combat.enemy_level = 1;
    rolls[2] = CFG_TEST_VICTORY_LOOT_FISH;
    game_roll_inject_begin(&game, rolls, 4);
    resolve_reply_out(&game, 1, &out);
    ASSERT_EQ(ITEM_FISH, game.corpse_item[WORLD_ROOM_CAMP][0]);
    PASS();
}

TEST combat_victory_can_leave_stripped_body(void)
{
    struct GameState game;
    GameEventQueue out;
    int rolls[3];

    unit_game_fresh(&game, 13u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 1;
    game.combat.enemy_level = 1;
    rolls[0] = 0;
    rolls[1] = CFG_TEST_VICTORY_LOOT_COUNT_NONE;
    rolls[2] = 0;
    game_roll_inject_begin(&game, rolls, 3);
    resolve_reply_out(&game, 1, &out);
    ASSERT_EQ(1, game.corpse_present[WORLD_ROOM_CAMP]);
    ASSERT_EQ(ITEM_NONE, game.corpse_item[WORLD_ROOM_CAMP][0]);
    ASSERT_EQ(ITEM_NONE, game.corpse_item[WORLD_ROOM_CAMP][1]);
    ASSERT_EQ(ITEM_NONE, game.corpse_item[WORLD_ROOM_CAMP][2]);
    PASS();
}

TEST combat_victory_can_drop_three_items(void)
{
    struct GameState game;
    GameEventQueue out;
    int rolls[6];

    unit_game_fresh(&game, 14u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    game.combat.enemy_hp = 1;
    rolls[0] = 0;
    rolls[1] = CFG_TEST_VICTORY_LOOT_COUNT_THREE;
    rolls[2] = CFG_TEST_VICTORY_LOOT_STICK;
    rolls[3] = CFG_TEST_VICTORY_LOOT_BERRY;
    rolls[4] = CFG_TEST_VICTORY_LOOT_HERB;
    rolls[5] = 0;
    game_roll_inject_begin(&game, rolls, 6);
    resolve_reply_out(&game, 1, &out);
    ASSERT_EQ(1, game.corpse_present[WORLD_ROOM_CAMP]);
    ASSERT_EQ(ITEM_STICK, game.corpse_item[WORLD_ROOM_CAMP][0]);
    ASSERT_EQ(ITEM_BERRY, game.corpse_item[WORLD_ROOM_CAMP][1]);
    ASSERT_EQ(ITEM_HERB, game.corpse_item[WORLD_ROOM_CAMP][2]);
    PASS();
}

SUITE(combat) {
    RUN_TEST(combat_attack_bonus);
    RUN_TEST(combat_start_mode);
    RUN_TEST(combat_start_uses_enemy_level_scaling);
    RUN_TEST(combat_replay_menu_emits_menu_phase);
    RUN_TEST(combat_enemy_initiative_strikes_before_menu);
    RUN_TEST(combat_reply_defend_reduces_damage);
    RUN_TEST(combat_reply_salve_in_combat);
    RUN_TEST(combat_reply_salve_at_full_hp);
    RUN_TEST(combat_victory_loot_and_xp);
    RUN_TEST(combat_victory_xp_scales_with_enemy_level);
    RUN_TEST(combat_victory_clears_active_bandit_slot);
    RUN_TEST(combat_invalid_choice);
    RUN_TEST(combat_player_death);
    RUN_TEST(combat_loot_tiers);
    RUN_TEST(combat_victory_can_leave_stripped_body);
    RUN_TEST(combat_victory_can_drop_three_items);
}
