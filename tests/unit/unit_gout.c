#include "greatest.h"
#include "config.h"
#include "command.h"
#include "gout.h"
#include "items.h"

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

TEST game_event_push_records_command_nav_kinds(void)
{
    GameEventQueue out;

    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_MAP, 0, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_HELP, CMD_HELP_TOPIC_MOVE, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_WAIT, 0, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_CANNOT_MOVE, 0, 0, 0, 0, "east"));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_UNKNOWN_COMMAND, 0, 0, 0, 0, 0));
    ASSERT_EQ(5, out.count);
    ASSERT_EQ(GAME_EVENT_MAP, out.events[0].kind);
    ASSERT_EQ(GAME_EVENT_HELP, out.events[1].kind);
    ASSERT_EQ(CMD_HELP_TOPIC_MOVE, out.events[1].arg0);
    ASSERT_EQ(GAME_EVENT_WAIT, out.events[2].kind);
    ASSERT_EQ(GAME_EVENT_CANNOT_MOVE, out.events[3].kind);
    ASSERT_STR_EQ("east", out.events[3].text);
    ASSERT_EQ(GAME_EVENT_UNKNOWN_COMMAND, out.events[4].kind);
    PASS();
}

TEST game_event_push_records_inventory_kinds(void)
{
    GameEventQueue out;

    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_ITEM_RESULT,
        GAME_ITEM_ACTION_TAKE, GAME_ITEM_OUTCOME_OK, 42, 5, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_BAG_VIEW, 0, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_CRAFT_RESULT,
        ITEM_TORCH, GAME_CRAFT_OUTCOME_OK, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_EQUIP_RESULT,
        ITEM_STICK, GAME_EQUIP_OUTCOME_WIELDED, 0, 0, 0));
    ASSERT_EQ(4, out.count);
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ITEM_ACTION_TAKE, out.events[0].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(42, out.events[0].arg2);
    ASSERT_EQ(5, out.events[0].arg3);
    ASSERT_EQ(GAME_EVENT_BAG_VIEW, out.events[1].kind);
    ASSERT_EQ(GAME_EVENT_CRAFT_RESULT, out.events[2].kind);
    ASSERT_EQ(ITEM_TORCH, out.events[2].arg0);
    ASSERT_EQ(GAME_CRAFT_OUTCOME_OK, out.events[2].arg1);
    ASSERT_EQ(GAME_EVENT_EQUIP_RESULT, out.events[3].kind);
    ASSERT_EQ(ITEM_STICK, out.events[3].arg0);
    ASSERT_EQ(GAME_EQUIP_OUTCOME_WIELDED, out.events[3].arg1);
    PASS();
}

TEST game_event_push_records_combat_progression_kinds(void)
{
    GameEventQueue out;

    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_COMBAT,
        GAME_COMBAT_PHASE_START, 10, 20, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_XP_GAIN, 7, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_STAT_CHANGE,
        2, 14, 1, 6, 0));
    ASSERT_EQ(3, out.count);
    ASSERT_EQ(GAME_EVENT_COMBAT, out.events[0].kind);
    ASSERT_EQ(GAME_COMBAT_PHASE_START, out.events[0].arg0);
    ASSERT_EQ(10, out.events[0].arg1);
    ASSERT_EQ(20, out.events[0].arg2);
    ASSERT_EQ(GAME_EVENT_XP_GAIN, out.events[1].kind);
    ASSERT_EQ(7, out.events[1].arg0);
    ASSERT_EQ(GAME_EVENT_STAT_CHANGE, out.events[2].kind);
    ASSERT_EQ(2, out.events[2].arg0);
    ASSERT_EQ(14, out.events[2].arg1);
    PASS();
}

TEST game_event_push_records_dialogue_encounter_kinds(void)
{
    GameEventQueue out;

    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_DIALOGUE,
        GAME_DIALOGUE_ACTOR_FROG, GAME_DIALOGUE_PHASE_BRANCH, 2, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_ENCOUNTER,
        GAME_ENCOUNTER_BANDIT, GAME_ENCOUNTER_ACTION_GIVE,
        GAME_ENCOUNTER_OUTCOME_OK, ITEM_STICK, "stick"));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_DIALOGUE_GUARD,
        GAME_DIALOGUE_GUARD_PICK_123, 0, 0, 0, 0));
    ASSERT_EQ(3, out.count);
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_FROG, out.events[0].arg0);
    ASSERT_EQ(2, out.events[0].arg2);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[1].kind);
    ASSERT_EQ(GAME_ENCOUNTER_BANDIT, out.events[1].arg0);
    ASSERT_EQ(ITEM_STICK, out.events[1].arg3);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[2].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_PICK_123, out.events[2].arg0);
    PASS();
}

SUITE(gout) {
    RUN_TEST(gout_reset_clears_state);
    RUN_TEST(gout_push_records_event_fields);
    RUN_TEST(gout_push_ignores_null_output);
    RUN_TEST(gout_push_marks_overflow_at_capacity);
    RUN_TEST(game_event_push_records_generic_move);
    RUN_TEST(game_event_push_records_command_nav_kinds);
    RUN_TEST(game_event_push_records_inventory_kinds);
    RUN_TEST(game_event_push_records_combat_progression_kinds);
    RUN_TEST(game_event_push_records_dialogue_encounter_kinds);
}
