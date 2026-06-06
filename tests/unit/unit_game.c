#include <string.h>
#include "greatest.h"
#include "config.h"
#include "game.h"
#include "genc.h"
#include "grendr.h"
#include "invent.h"
#include "items.h"
#include "testharn.h"
#include "unit_util.h"

static int run_cmd(struct GameState *game, const char *line)
{
    GameEventQueue out;
    char buf[CFG_INPUT_MAX];

    game_event_queue_reset(&out);
    strncpy(buf, line, CFG_INPUT_MAX - 1);
    buf[CFG_INPUT_MAX - 1] = '\0';
    return game_process_input(game, buf, &out);
}

TEST game_heal_player_applies(void)
{
    struct GameState game;

    unit_game_fresh(&game, 30u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.player_hp = 5;
    ASSERT_EQ(1, game_heal_player(&game, CFG_SALVE_HEAL_AMOUNT));
    ASSERT_EQ(10, game.player_hp);
    PASS();
}

TEST game_heal_player_at_max(void)
{
    struct GameState game;

    unit_game_fresh(&game, 31u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(0, game_heal_player(&game, CFG_BERRY_HEAL_AMOUNT));
    ASSERT_EQ(CFG_START_MAX_HP, game.player_hp);
    PASS();
}

TEST game_heal_player_clamps(void)
{
    struct GameState game;

    unit_game_fresh(&game, 32u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.player_hp = CFG_START_MAX_HP - 1;
    ASSERT_EQ(1, game_heal_player(&game, CFG_FISH_HEAL_AMOUNT));
    ASSERT_EQ(CFG_START_MAX_HP, game.player_hp);
    PASS();
}

TEST game_mode_setters(void)
{
    struct GameState game;

    unit_game_fresh(&game, 1u);
    game_set_mode_dialogue(&game, DIALOGUE_NPC_FROG);
    ASSERT_EQ(1, game_is_busy_dialogue(&game));
    game_set_mode_combat(&game);
    ASSERT_EQ(1, game_is_busy_dialogue(&game));
    game_set_mode_explore(&game);
    ASSERT_EQ(0, game_is_busy_dialogue(&game));
    PASS();
}

TEST game_describe_current_room_emits_look(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 33u);
    game_event_queue_reset(&out);
    game_describe_current_room(&game, &out);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ROOM_LOOK, out.events[0].kind);
    PASS();
}

TEST game_describe_current_room_overflow_keeps_prior_event(void)
{
    struct GameState game;
    GameEventQueue out;
    int i;

    unit_game_fresh(&game, 36u);
    game_event_queue_reset(&out);
    for (i = 0; i < CFG_GAME_EVENT_MAX; ++i) {
        ASSERT(0 != game_event_push(&out, GAME_EVENT_WAIT, i, 0, 0, 0, 0));
    }
    out.events[out.count - 1].room_id = 77;
    out.events[out.count - 1].room_item[0] = ITEM_STICK;

    game_describe_current_room(&game, &out);

    ASSERT_EQ(CFG_GAME_EVENT_MAX, out.count);
    ASSERT_EQ(1, out.overflowed);
    ASSERT_EQ(GAME_EVENT_WAIT, out.events[out.count - 1].kind);
    ASSERT_EQ(CFG_GAME_EVENT_MAX - 1, out.events[out.count - 1].arg0);
    ASSERT_EQ(77, out.events[out.count - 1].room_id);
    ASSERT_EQ(ITEM_STICK, out.events[out.count - 1].room_item[0]);
    PASS();
}

TEST game_roll_inject_consume(void)
{
    struct GameState game;
    int rolls[2];

    unit_game_fresh(&game, 2u);
    rolls[0] = 3;
    rolls[1] = 7;
    game_roll_inject_begin(&game, rolls, 2);
    ASSERT_EQ(3, game_roll_spread(&game, 10));
    ASSERT_EQ(7, game_roll_spread(&game, 10));
    ASSERT_EQ(1, game_roll_inject_fully_consumed(&game));
    game_roll_inject_clear(&game);
    ASSERT_EQ(1, game_roll_inject_fully_consumed(&game));
    PASS();
}

TEST game_move_blocked_and_ok(void)
{
    struct GameState game;

    unit_game_fresh(&game, 3u);
    ASSERT_EQ(0, run_cmd(&game, "move east"));
    ASSERT_EQ(1, run_cmd(&game, "move north"));
    ASSERT_EQ(WORLD_ROOM_ROAD, game.player.room_id);
    PASS();
}

TEST game_quiet_ticks(void)
{
    struct GameState game;
    u32 tick_before;

    unit_game_fresh(&game, 4u);
    game.test_quiet_ticks = 1;
    game.wanderer_active = 0;
    tick_before = game.tick;
    {
        GameEventQueue out;

        game_event_queue_reset(&out);
        game_background_step(&game, &out);
    }
    ASSERT_EQ(tick_before + 1, game.tick);
    PASS();
}

TEST game_bandit_intimidate_success(void)
{
    struct GameState game;
    int rolls[1];
    char line[] = "3";

    unit_game_fresh(&game, 5u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    {
        GameEventQueue out;

        game_event_queue_reset(&out);
        enemy_begin_encounter(&game, &out);
    }
    rolls[0] = CFG_TEST_INTIMIDATE_OK;
    game_roll_inject_begin(&game, rolls, 1);
    {
        GameEventQueue out;

        game_event_queue_reset(&out);
        game_process_input(&game, line, &out);
    }
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST game_inspect_with_focus(void)
{
    struct GameState game;

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.env_focus_active = 1;
    game.env_focus_room = WORLD_ROOM_CAMP;
    game.env_focus_kind = GAME_ENV_WATER;
    game.env_focus_expires_tick = game.tick + 10;
    ASSERT_EQ(1, run_cmd(&game, "inspect water"));
    ASSERT_EQ(0, game.env_focus_active);
    PASS();
}

TEST game_talk_frog(void)
{
    struct GameState game;

    unit_game_fresh(&game, 7u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    ASSERT_EQ(1, run_cmd(&game, "talk"));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_FROG, game.dialogue);
    PASS();
}

TEST game_bandit_fight_reply(void)
{
    struct GameState game;
    char line[] = "1";

    unit_game_fresh(&game, 10u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    {
        GameEventQueue out;

        game_event_queue_reset(&out);
        enemy_begin_encounter(&game, &out);
        game_event_queue_reset(&out);
        game_process_input(&game, line, &out);
    }
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    PASS();
}

TEST game_bandit_intimidate_fail(void)
{
    struct GameState game;
    int rolls[1];
    char line[] = "3";

    unit_game_fresh(&game, 11u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    {
        GameEventQueue out;

        game_event_queue_reset(&out);
        enemy_begin_encounter(&game, &out);
    }
    rolls[0] = CFG_TEST_INTIMIDATE_FAIL;
    game_roll_inject_begin(&game, rolls, 1);
    {
        GameEventQueue out;

        game_event_queue_reset(&out);
        game_process_input(&game, line, &out);
    }
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    PASS();
}

TEST game_bandit_handover_pick(void)
{
    struct GameState game;
    char line[] = "2";

    unit_game_fresh(&game, 12u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_STICK);
    {
        GameEventQueue out;

        game_event_queue_reset(&out);
        enemy_begin_encounter(&game, &out);
        game_event_queue_reset(&out);
        game_process_input(&game, line, &out);
    }
    ASSERT_EQ(1, game.enemy_handover_pick);
    PASS();
}

TEST game_talk_npcs_and_nobody(void)
{
    struct GameState game;

    unit_game_fresh(&game, 13u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, run_cmd(&game, "talk"));
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    ASSERT_EQ(1, run_cmd(&game, "talk"));
    ASSERT_EQ(DIALOGUE_NPC_WATCHMAN, game.dialogue);
    PASS();
}

TEST game_frog_reply_branch(void)
{
    struct GameState game;

    unit_game_fresh(&game, 14u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    run_cmd(&game, "talk");
    ASSERT_EQ(1, run_cmd(&game, "reply 1"));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST game_combat_blocks_inventory_cmds(void)
{
    struct GameState game;

    unit_game_fresh(&game, 15u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    ASSERT_EQ(0, run_cmd(&game, "take stick"));
    ASSERT_EQ(1, run_cmd(&game, "look"));
    PASS();
}

TEST game_inspect_none_and_wrong(void)
{
    struct GameState game;

    unit_game_fresh(&game, 16u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, run_cmd(&game, "inspect"));
    game.env_focus_active = 1;
    game.env_focus_room = WORLD_ROOM_CAMP;
    game.env_focus_kind = GAME_ENV_RUSTLE;
    game.env_focus_expires_tick = game.tick + 10;
    ASSERT_EQ(1, run_cmd(&game, "inspect water"));
    PASS();
}

TEST game_unknown_command(void)
{
    struct GameState game;

    unit_game_fresh(&game, 17u);
    ASSERT_EQ(0, run_cmd(&game, "flibble"));
    PASS();
}

TEST game_give_after_handover_fixture(void)
{
    struct GameState game;

    unit_game_fresh(&game, 19u);
    testharn_apply(&game, "@fixture bandit_handover_pick");
    ASSERT_EQ(1, run_cmd(&game, "give stick"));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST game_wanderer_reply_fixture(void)
{
    struct GameState game;

    unit_game_fresh(&game, 20u);
    testharn_apply(&game, "@fixture wanderer_dialogue");
    ASSERT_EQ(1, run_cmd(&game, "reply 2"));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(0, game.wanderer_active);
    PASS();
}

TEST game_wait_and_help(void)
{
    struct GameState game;

    unit_game_fresh(&game, 21u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.test_quiet_ticks = 1;
    game.wanderer_active = 0;
    ASSERT_EQ(1, run_cmd(&game, "wait"));
    ASSERT_EQ(1, run_cmd(&game, "help move"));
    PASS();
}

TEST game_session_help_no_tick(void)
{
    struct GameState game;

    unit_game_fresh(&game, 23u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.test_quiet_ticks = 1;
    game.wanderer_active = 0;
    ASSERT_EQ(0, game.tick);
    ASSERT_EQ(1, run_cmd(&game, "help move"));
    ASSERT_EQ(0, game.tick);
    PASS();
}

TEST game_observe_look_no_tick(void)
{
    struct GameState game;

    unit_game_fresh(&game, 24u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.test_quiet_ticks = 1;
    game.wanderer_active = 0;
    ASSERT_EQ(0, game.tick);
    ASSERT_EQ(1, run_cmd(&game, "look"));
    ASSERT_EQ(0, game.tick);
    PASS();
}

TEST game_pass_time_wait_ticks(void)
{
    struct GameState game;

    unit_game_fresh(&game, 25u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.test_quiet_ticks = 1;
    game.wanderer_active = 0;
    ASSERT_EQ(0, game.tick);
    ASSERT_EQ(1, run_cmd(&game, "wait"));
    ASSERT_EQ(1, game.tick);
    PASS();
}

TEST game_wait_emits_output_record(void)
{
    struct GameState game;
    GameEventQueue out;
    char line[] = "wait";

    unit_game_fresh(&game, 34u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.test_quiet_ticks = 1;
    game.wanderer_active = 0;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, game_process_input(&game, line, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_WAIT, out.events[0].kind);
    PASS();
}

TEST game_help_emits_help_event(void)
{
    struct GameState game;
    GameEventQueue out;
    char line[] = "help move";

    unit_game_fresh(&game, 37u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.test_quiet_ticks = 1;
    game.wanderer_active = 0;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, game_process_input(&game, line, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_HELP, out.events[0].kind);
    ASSERT_EQ(CMD_HELP_TOPIC_MOVE, out.events[0].arg0);
    PASS();
}

TEST game_map_emits_map_event(void)
{
    struct GameState game;
    GameEventQueue out;
    char line[] = "map";

    unit_game_fresh(&game, 38u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.test_quiet_ticks = 1;
    game.wanderer_active = 0;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, game_process_input(&game, line, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_MAP, out.events[0].kind);
    PASS();
}

TEST game_unknown_command_emits_event(void)
{
    struct GameState game;
    GameEventQueue out;
    char line[] = "flibble";

    unit_game_fresh(&game, 39u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_event_queue_reset(&out);
    ASSERT_EQ(0, game_process_input(&game, line, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_UNKNOWN_COMMAND, out.events[0].kind);
    PASS();
}

TEST game_cannot_move_emits_event(void)
{
    struct GameState game;
    GameEventQueue out;
    char line[] = "move east";

    unit_game_fresh(&game, 40u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.test_quiet_ticks = 1;
    game.wanderer_active = 0;
    game_event_queue_reset(&out);
    ASSERT_EQ(0, game_process_input(&game, line, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_CANNOT_MOVE, out.events[0].kind);
    ASSERT_STR_EQ("east", out.events[0].text);
    PASS();
}

TEST game_move_emits_move_then_look(void)
{
    struct GameState game;
    GameEventQueue out;
    char line[] = "move north";

    unit_game_fresh(&game, 35u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.test_quiet_ticks = 1;
    game.wanderer_active = 0;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, game_process_input(&game, line, &out));
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_MOVE, out.events[0].kind);
    ASSERT_STR_EQ("north", out.events[0].text);
    ASSERT_EQ(GAME_EVENT_ROOM_LOOK, out.events[1].kind);
    PASS();
}

TEST game_roll_spread_zero(void)
{
    struct GameState game;

    unit_game_fresh(&game, 22u);
    ASSERT_EQ(0, game_roll_spread(&game, 0));
    PASS();
}

TEST game_quit_ends_run(void)
{
    struct GameState game;

    unit_game_fresh(&game, 18u);
    ASSERT_EQ(1, run_cmd(&game, "quit"));
    ASSERT_EQ(0, game.running);
    PASS();
}

TEST game_bandit_waiting_reply_guard_event(void)
{
    struct GameState game;
    GameEventQueue out;
    char line[] = "wait";

    unit_game_fresh(&game, 40u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_ENEMY);
    game_event_queue_reset(&out);
    ASSERT_EQ(0, game_process_input(&game, line, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_BANDIT_WAITING_REPLY, out.events[0].arg0);
    PASS();
}

TEST game_nobody_waiting_reply_guard_event(void)
{
    struct GameState game;
    GameEventQueue out;
    char line[] = "1";

    unit_game_fresh(&game, 41u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, game_process_input(&game, line, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_NOBODY_WAITING_REPLY, out.events[0].arg0);
    PASS();
}

SUITE(game) {
    RUN_TEST(game_heal_player_applies);
    RUN_TEST(game_heal_player_at_max);
    RUN_TEST(game_heal_player_clamps);
    RUN_TEST(game_mode_setters);
    RUN_TEST(game_describe_current_room_emits_look);
    RUN_TEST(game_describe_current_room_overflow_keeps_prior_event);
    RUN_TEST(game_roll_inject_consume);
    RUN_TEST(game_move_blocked_and_ok);
    RUN_TEST(game_quiet_ticks);
    RUN_TEST(game_bandit_intimidate_success);
    RUN_TEST(game_inspect_with_focus);
    RUN_TEST(game_talk_frog);
    RUN_TEST(game_bandit_fight_reply);
    RUN_TEST(game_bandit_intimidate_fail);
    RUN_TEST(game_bandit_handover_pick);
    RUN_TEST(game_talk_npcs_and_nobody);
    RUN_TEST(game_frog_reply_branch);
    RUN_TEST(game_combat_blocks_inventory_cmds);
    RUN_TEST(game_inspect_none_and_wrong);
    RUN_TEST(game_unknown_command);
    RUN_TEST(game_give_after_handover_fixture);
    RUN_TEST(game_wanderer_reply_fixture);
    RUN_TEST(game_wait_and_help);
    RUN_TEST(game_session_help_no_tick);
    RUN_TEST(game_observe_look_no_tick);
    RUN_TEST(game_pass_time_wait_ticks);
    RUN_TEST(game_wait_emits_output_record);
    RUN_TEST(game_help_emits_help_event);
    RUN_TEST(game_map_emits_map_event);
    RUN_TEST(game_unknown_command_emits_event);
    RUN_TEST(game_cannot_move_emits_event);
    RUN_TEST(game_move_emits_move_then_look);
    RUN_TEST(game_roll_spread_zero);
    RUN_TEST(game_quit_ends_run);
    RUN_TEST(game_bandit_waiting_reply_guard_event);
    RUN_TEST(game_nobody_waiting_reply_guard_event);
}
