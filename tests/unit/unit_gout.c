#include "greatest.h"
#include "config.h"
#include "gout.h"

TEST gout_reset_clears_state(void)
{
    GameEventQueue out;

    out.count = 7;
    out.overflowed = 1;
    gout_reset(&out);
    ASSERT_EQ(0, out.count);
    ASSERT_EQ(0, out.overflowed);
    PASS();
}

TEST gout_push_records_event_fields(void)
{
    GameEventQueue out;

    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push_legacy(&out, GAME_OUT_MSG_WAIT, 1, 2, 3, 4, "txt"));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(0, out.overflowed);
    ASSERT_EQ(GAME_EVENT_LEGACY, out.events[0].kind);
    ASSERT_EQ(GAME_OUT_MSG_WAIT, out.events[0].legacy_kind);
    ASSERT_EQ(1, out.events[0].arg0);
    ASSERT_EQ(2, out.events[0].arg1);
    ASSERT_EQ(3, out.events[0].arg2);
    ASSERT_EQ(4, out.events[0].arg3);
    ASSERT_EQ(-1, out.events[0].room_id);
    ASSERT_EQ(0, out.events[0].room_item[0]);
    ASSERT_STR_EQ("txt", out.events[0].text);
    PASS();
}

TEST gout_push_ignores_null_output(void)
{
    ASSERT(0 == game_event_push_legacy(0, GAME_OUT_MSG_WAIT, 0, 0, 0, 0, 0));
    PASS();
}

TEST gout_push_marks_overflow_at_capacity(void)
{
    GameEventQueue out;
    int i;

    game_event_queue_reset(&out);
    for (i = 0; i < CFG_GAME_OUT_MAX; ++i) {
        ASSERT(0 != game_event_push_legacy(&out, GAME_OUT_MSG_WAIT, i, 0, 0, 0, 0));
    }
    ASSERT_EQ(CFG_GAME_OUT_MAX, out.count);
    ASSERT_EQ(0, out.overflowed);
    ASSERT(0 == game_event_push_legacy(&out, GAME_OUT_MSG_WAIT, 99, 0, 0, 0, 0));
    ASSERT_EQ(CFG_GAME_OUT_MAX, out.count);
    ASSERT_EQ(1, out.overflowed);
    PASS();
}

TEST game_event_push_records_generic_move(void)
{
    GameEventQueue out;
    GameEvent *ev;

    game_event_queue_reset(&out);
    ev = game_event_push(&out, GAME_EVENT_MOVE, 0, 0, 0, 0, "north");
    ASSERT(0 != ev);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_MOVE, out.events[0].kind);
    ASSERT_EQ(GAME_OUT_NONE, out.events[0].legacy_kind);
    ASSERT_STR_EQ("north", out.events[0].text);
    PASS();
}

SUITE(gout) {
    RUN_TEST(gout_reset_clears_state);
    RUN_TEST(gout_push_records_event_fields);
    RUN_TEST(gout_push_ignores_null_output);
    RUN_TEST(gout_push_marks_overflow_at_capacity);
    RUN_TEST(game_event_push_records_generic_move);
}
