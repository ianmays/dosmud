/*
 * unit_rend: direct grendr slice-budget checks (#207). These cover event drain
 * plus HUD only; shell-owned spacer/status rows from main_emit stay in main.c
 * and are covered by regression snapshots such as safe_output_budget.
 */
#include "greatest.h"
#include "config.h"
#include "game.h"
#include "gout.h"
#include "grendr.h"
#include "items.h"
#include "world.h"
#include "unit_util.h"

/* Same grendr-side composition as main: begin, drain events, then HUD. */
static int render_count_frame_lines(struct GameState *game,
                                    const GameEventQueue *out)
{
    render_frame_begin();
    game_render_output(game, out);
    game_render(game);
    return render_frame_line_count();
}

/* #244: empty step still counts render_gap + HUD as two grendr rows. */
TEST render_blank_hud_frame_counts_gap_and_hud(void)
{
    struct GameState game;

    unit_game_fresh(&game, 1234u);
    render_frame_begin();
    game_render(&game);
    ASSERT_EQ(2, render_frame_line_count());
    ASSERT_EQ(0, render_frame_over_budget());
    PASS();
}

TEST render_room_look_frame_stays_within_safe_budget(void)
{
    struct GameState game;
    GameEventQueue out;
    int lines;

    unit_game_fresh(&game, 1234u);
    game_event_queue_reset(&out);
    game_describe_current_room(&game, &out);
    lines = render_count_frame_lines(&game, &out);
    ASSERT(lines <= CFG_SAFE_OUTPUT_MAX_LINES);
    ASSERT_EQ(0, render_frame_over_budget());
    PASS();
}

TEST render_map_frame_stays_within_safe_budget(void)
{
    struct GameState game;
    GameEventQueue out;
    int lines;

    unit_game_fresh(&game, 1234u);
    game.room_explored[WORLD_ROOM_CAMP] = 1;
    game.room_explored[WORLD_ROOM_ROAD] = 1;
    game.room_explored[WORLD_ROOM_POND] = 1;
    game.room_explored[WORLD_ROOM_FOREST] = 1;
    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_MAP, 0, 0, 0, 0, 0));
    lines = render_count_frame_lines(&game, &out);
    ASSERT(lines <= CFG_SAFE_OUTPUT_MAX_LINES);
    ASSERT_EQ(0, render_frame_over_budget());
    PASS();
}

TEST render_bandit_open_frame_stays_within_safe_budget(void)
{
    struct GameState game;
    GameEventQueue out;
    int lines;

    unit_game_fresh(&game, 1234u);
    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_ENCOUNTER,
        GAME_ENCOUNTER_BANDIT, GAME_ENCOUNTER_ACTION_OPEN,
        GAME_ENCOUNTER_OUTCOME_NONE, 2, 0));
    lines = render_count_frame_lines(&game, &out);
    ASSERT(lines <= CFG_SAFE_OUTPUT_MAX_LINES);
    ASSERT_EQ(0, render_frame_over_budget());
    PASS();
}

TEST render_corpse_menu_frame_stays_within_safe_budget(void)
{
    struct GameState game;
    GameEventQueue out;
    GameEvent *ev;
    int lines;

    unit_game_fresh(&game, 1234u);
    game_event_queue_reset(&out);
    ev = game_event_push(&out, GAME_EVENT_CORPSE_VIEW, 3, 4, 0, 0, 0);
    ASSERT(0 != ev);
    ev->room_id = WORLD_ROOM_ROAD;
    ev->room_item[0] = ITEM_SPEAR;
    ev->room_item[1] = ITEM_BERRY;
    ev->room_item[2] = ITEM_HERB;
    lines = render_count_frame_lines(&game, &out);
    ASSERT(lines <= CFG_SAFE_OUTPUT_MAX_LINES);
    ASSERT_EQ(0, render_frame_over_budget());
    PASS();
}

TEST render_player_defeat_frame_stays_within_safe_budget(void)
{
    struct GameState game;
    GameEventQueue out;
    GameEvent *ev;
    int lines;

    unit_game_fresh(&game, 206u);
    game_event_queue_reset(&out);
    ev = game_event_push(&out, GAME_EVENT_PLAYER_DEFEAT, 25, 4, 3, 3, 0);
    ASSERT(0 != ev);
    ev->room_id = WORLD_ROOM_ROAD;
    ev->room_item[0] = ITEM_SPEAR;
    ev->room_item[1] = ITEM_MARSH_ROOT;
    ev->room_item[2] = 1;
    ev->room_item[3] = 2;
    lines = render_count_frame_lines(&game, &out);
    ASSERT(lines <= CFG_SAFE_OUTPUT_MAX_LINES);
    ASSERT_EQ(0, render_frame_over_budget());
    PASS();
}

TEST render_player_corpse_menu_stays_within_safe_budget(void)
{
    struct GameState game;
    GameEventQueue out;
    GameEvent *ev;
    int lines;

    unit_game_fresh(&game, 207u);
    game_event_queue_reset(&out);
    ev = game_event_push(&out, GAME_EVENT_CORPSE_VIEW, 3, 4,
        GAME_CORPSE_KIND_PLAYER, 0, 0);
    ASSERT(0 != ev);
    ev->room_id = WORLD_ROOM_ROAD;
    ev->room_item[0] = ITEM_SPEAR;
    ev->room_item[1] = ITEM_BERRY;
    ev->room_item[2] = ITEM_HERB;
    lines = render_count_frame_lines(&game, &out);
    ASSERT(lines <= CFG_SAFE_OUTPUT_MAX_LINES);
    ASSERT_EQ(0, render_frame_over_budget());
    PASS();
}

TEST render_player_corpse_menu_adds_gap_before_look_flavor(void)
{
    struct GameState game;
    GameEventQueue look_out;
    GameEventQueue out;
    GameEvent *ev;
    int menu_lines;
    int look_lines;
    int combined_lines;

    unit_game_fresh(&game, 208u);
    game.weather_kind = GAME_WEATHER_RAIN;
    game.env_room_clues[WORLD_ROOM_CAMP] =
        (u8)GAME_ENV_CLUE_BIT(GAME_ENV_RUSTLE);

    game_event_queue_reset(&out);
    ev = game_event_push(&out, GAME_EVENT_CORPSE_VIEW, 1, 2,
        GAME_CORPSE_KIND_PLAYER, 0, 0);
    ASSERT(0 != ev);
    ev->room_id = WORLD_ROOM_ROAD;
    ev->room_item[0] = ITEM_SPEAR;
    menu_lines = render_count_frame_lines(&game, &out);

    game_event_queue_reset(&look_out);
    ASSERT(0 != game_event_push(&look_out, GAME_EVENT_ENVIRONMENT,
        GAME_ENV_EVENT_WEATHER_RAIN, 0, 0, 0, 0));
    game_describe_current_room(&game, &look_out);
    look_lines = render_count_frame_lines(&game, &look_out);

    ASSERT(0 != game_event_push(&out, GAME_EVENT_ENVIRONMENT,
        GAME_ENV_EVENT_WEATHER_RAIN, 0, 0, 0, 0));
    game_describe_current_room(&game, &out);
    combined_lines = render_count_frame_lines(&game, &out);
    /* Separate frames duplicate two HUD rows; the combined frame adds one gap. */
    ASSERT_EQ(menu_lines + look_lines - 1, combined_lines);
    PASS();
}

TEST render_item_result_adds_gap_before_compact_arrival_flavor(void)
{
    struct GameState game;
    GameEventQueue move_out;
    GameEventQueue out;
    GameEvent *ev;
    int result_lines;
    int move_lines;
    int combined_lines;

    unit_game_fresh(&game, 209u);
    game.weather_kind = GAME_WEATHER_RAIN;

    game_event_queue_reset(&out);
    ev = game_event_push(&out, GAME_EVENT_ITEM_RESULT,
        GAME_ITEM_ACTION_LOOT, GAME_ITEM_OUTCOME_LEFT_BEHIND, ITEM_NONE, 0, 0);
    ASSERT(0 != ev);
    result_lines = render_count_frame_lines(&game, &out);

    game_event_queue_reset(&move_out);
    ev = game_event_push(&move_out, GAME_EVENT_MOVE, 0, 0, 0, 0, 0);
    ASSERT(0 != ev);
    ev->text = "north";
    game.player.room_id = WORLD_ROOM_ROAD;
    game_describe_current_room_tight(&game, &move_out);
    move_lines = render_count_frame_lines(&game, &move_out);

    ASSERT(0 != game_event_push(&out, GAME_EVENT_MOVE, 0, 0, 0, 0, 0));
    out.events[out.count - 1].text = "north";
    game_describe_current_room_tight(&game, &out);
    combined_lines = render_count_frame_lines(&game, &out);

    /* Separate frames duplicate two HUD rows; the combined frame adds one gap. */
    ASSERT_EQ(result_lines + move_lines - 1, combined_lines);
    PASS();
}

SUITE(grendr)
{
    RUN_TEST(render_blank_hud_frame_counts_gap_and_hud);
    RUN_TEST(render_room_look_frame_stays_within_safe_budget);
    RUN_TEST(render_map_frame_stays_within_safe_budget);
    RUN_TEST(render_bandit_open_frame_stays_within_safe_budget);
    RUN_TEST(render_corpse_menu_frame_stays_within_safe_budget);
    RUN_TEST(render_player_defeat_frame_stays_within_safe_budget);
    RUN_TEST(render_player_corpse_menu_stays_within_safe_budget);
    RUN_TEST(render_player_corpse_menu_adds_gap_before_look_flavor);
    RUN_TEST(render_item_result_adds_gap_before_compact_arrival_flavor);
}
