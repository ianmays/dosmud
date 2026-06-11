#include <string.h>
#include <time.h>
#include "greatest.h"
#include "config.h"
#include "game.h"
#include "combat.h"
#include "genc.h"
#include "grendr.h"
#include "npc.h"
#include "unit_util.h"
#include "soak_util.h"

static int soak_run_cmd(struct GameState *game, const char *line)
{
    GameEventQueue out;
    char buf[CFG_INPUT_MAX];

    game_event_queue_reset(&out);
    strncpy(buf, line, CFG_INPUT_MAX - 1);
    buf[CFG_INPUT_MAX - 1] = '\0';
    return game_process_input(game, buf, &out);
}

TEST soak_background_ticks(void)
{
    struct GameState game;
    GameEventQueue out;
    unsigned long i;
    clock_t start;
    clock_t end;

    unit_game_fresh(&game, 116u);
    render_set_suppress(1);
    ASSERT(soak_assert_game_state_ok(&game));
    start = clock();
    for (i = 1; i <= CFG_TEST_SOAK_TICKS; i++) {
        game_event_queue_reset(&out);
        game_background_step(&game, &out);
        ASSERT_EQ(0, out.overflowed);
        if ((i % CFG_TEST_SOAK_CHECK_INTERVAL) == 0) {
            ASSERT(soak_assert_game_state_ok(&game));
        }
    }
    end = clock();
    ASSERT(soak_assert_game_state_ok(&game));
    ASSERT(soak_check_limit("background_ticks",
        soak_print_bench("background_ticks", CFG_TEST_SOAK_TICKS, end - start)));
    PASS();
}

TEST soak_command_wait_move(void)
{
    struct GameState game;
    unsigned long i;
    clock_t start;
    clock_t end;

    unit_game_fresh(&game, 117u);
    render_set_suppress(1);
    game.test_quiet_ticks = 1;
    npc_deactivate_until(&game, GAME_DIALOGUE_ACTOR_TRAVELER, 999999UL);
    ASSERT(soak_assert_game_state_ok(&game));
    start = clock();
    for (i = 1; i <= CFG_TEST_SOAK_TICKS; i++) {
        if ((i & 1) != 0) {
            soak_run_cmd(&game, "wait");
        } else {
            soak_run_cmd(&game, "move north");
        }
        if ((i % CFG_TEST_SOAK_CHECK_INTERVAL) == 0) {
            ASSERT(soak_assert_game_state_ok(&game));
        }
    }
    end = clock();
    ASSERT(soak_assert_game_state_ok(&game));
    ASSERT(soak_check_limit("command_ticks",
        soak_print_bench("command_ticks", CFG_TEST_SOAK_TICKS, end - start)));
    PASS();
}

TEST soak_combat_loop(void)
{
    struct GameState game;
    GameEventQueue out;
    int rolls[3];
    unsigned long i;
    clock_t start;
    clock_t end;

    unit_game_fresh(&game, 118u);
    render_set_suppress(1);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT(soak_assert_game_state_ok(&game));
    start = clock();
    for (i = 1; i <= CFG_TEST_SOAK_COMBAT_ROUNDS; i++) {
        game_set_mode_combat(&game);
        game.combat.enemy_hp = 1;
        game.combat.defending = 0;
        rolls[0] = 0;
        rolls[1] = CFG_TEST_VICTORY_LOOT_BERRY;
        rolls[2] = CFG_TEST_VICTORY_XP_SPREAD;
        game_roll_inject_begin(&game, rolls, 3);
        game_event_queue_reset(&out);
        combat_resolve_reply(&game, 1, &out);
        ASSERT_EQ(0, out.overflowed);
        if ((i % CFG_TEST_SOAK_COMBAT_CHECK_INTERVAL) == 0) {
            ASSERT(soak_assert_game_state_ok(&game));
        }
        if (game.mode == GAME_MODE_EXPLORE && game.running) {
            game.corpse_present[WORLD_ROOM_CAMP] = 0;
            game.corpse_loot[WORLD_ROOM_CAMP] = 0;
        }
    }
    end = clock();
    ASSERT(soak_assert_game_state_ok(&game));
    ASSERT(soak_check_limit("combat_rounds",
        soak_print_bench("combat_rounds", CFG_TEST_SOAK_COMBAT_ROUNDS, end - start)));
    PASS();
}

SUITE(soak) {
    RUN_TEST(soak_background_ticks);
    RUN_TEST(soak_command_wait_move);
    RUN_TEST(soak_combat_loop);
}
