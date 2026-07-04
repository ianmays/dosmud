#include "greatest.h"
#include "config.h"
#include "command.h"
#include "game.h"
#include "gout.h"
#include "items.h"
#include "npc.h"
#include "txtres.h"

TEST game_event_queue_reset_clears_state(void)
{
    GameEventQueue out;

    out.count = 7;
    out.overflowed = 1;
    game_event_queue_reset(&out);
    ASSERT_EQ(0, out.count);
    ASSERT_EQ(0, out.overflowed);
    PASS();
}

TEST game_event_push_ignores_null_output(void)
{
    ASSERT(0 == game_event_push(0, GAME_EVENT_WAIT, 0, 0, 0, 0, 0));
    PASS();
}

TEST game_event_push_marks_overflow_at_capacity(void)
{
    GameEventQueue out;
    int i;

    game_event_queue_reset(&out);
    for (i = 0; i < CFG_GAME_EVENT_MAX; ++i) {
        ASSERT(0 != game_event_push(&out, GAME_EVENT_WAIT, i, 0, 0, 0, 0));
    }
    ASSERT_EQ(CFG_GAME_EVENT_MAX, out.count);
    ASSERT_EQ(0, out.overflowed);
    ASSERT(0 == game_event_push(&out, GAME_EVENT_WAIT, 99, 0, 0, 0, 0));
    ASSERT_EQ(CFG_GAME_EVENT_MAX, out.count);
    ASSERT_EQ(1, out.overflowed);
    PASS();
}

TEST game_event_queue_reset_clears_overflow_and_reuse_state(void)
{
    GameEventQueue out;
    GameEvent *ev;
    int i;

    game_event_queue_reset(&out);
    for (i = 0; i < CFG_GAME_EVENT_MAX; ++i) {
        ASSERT(0 != game_event_push(&out, GAME_EVENT_WAIT, i, 0, 0, 0, 0));
    }
    ASSERT(0 == game_event_push(&out, GAME_EVENT_WAIT, 99, 0, 0, 0, 0));
    ASSERT_EQ(1, out.overflowed);

    game_event_queue_reset(&out);
    ev = game_event_push(&out, GAME_EVENT_HELP, CMD_HELP_TOPIC_TALK, 0, 0, 0, 0);
    ASSERT(0 != ev);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(0, out.overflowed);
    ASSERT_EQ(GAME_EVENT_HELP, out.events[0].kind);
    ASSERT_EQ(CMD_HELP_TOPIC_TALK, out.events[0].arg0);
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
    ASSERT_STR_EQ("north", out.events[0].text);
    PASS();
}

TEST game_event_push_preserves_enqueue_order_and_payload_slots(void)
{
    GameEventQueue out;

    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_MOVE, 0, 0, 0, 0, "north"));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_ITEM_RESULT,
        GAME_ITEM_ACTION_USE, GAME_ITEM_OUTCOME_OK, ITEM_SALVE, 10, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_DIALOGUE,
        GAME_DIALOGUE_ACTOR_TRAVELER, GAME_DIALOGUE_PHASE_REPLY, 2, 0, 0));

    ASSERT_EQ(3, out.count);
    ASSERT_EQ(GAME_EVENT_MOVE, out.events[0].kind);
    ASSERT_STR_EQ("north", out.events[0].text);
    ASSERT_EQ(-1, out.events[0].room_id);
    ASSERT_EQ(0, out.events[0].room_item[0]);

    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[1].kind);
    ASSERT_EQ(GAME_ITEM_ACTION_USE, out.events[1].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[1].arg1);
    ASSERT_EQ(ITEM_SALVE, out.events[1].arg2);
    ASSERT_EQ(10, out.events[1].arg3);
    ASSERT(0 == out.events[1].text);

    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[2].kind);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_TRAVELER, out.events[2].arg0);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_REPLY, out.events[2].arg1);
    ASSERT_EQ(2, out.events[2].arg2);
    PASS();
}

TEST game_event_push_records_command_nav_kinds(void)
{
    GameEventQueue out;

    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_MAP, 0, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_HELP, CMD_HELP_TOPIC_MOVE, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_VERSION, 0, 0, 0, 0, TXT_MAIN_VERSION_FMT));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_WAIT, 0, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_CANNOT_MOVE, 0, 0, 0, 0, "east"));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_UNKNOWN_COMMAND, 0, 0, 0, 0, 0));
    ASSERT_EQ(6, out.count);
    ASSERT_EQ(GAME_EVENT_MAP, out.events[0].kind);
    ASSERT_EQ(GAME_EVENT_HELP, out.events[1].kind);
    ASSERT_EQ(CMD_HELP_TOPIC_MOVE, out.events[1].arg0);
    ASSERT_EQ(GAME_EVENT_VERSION, out.events[2].kind);
    ASSERT_STR_EQ(TXT_MAIN_VERSION_FMT, out.events[2].text);
    ASSERT_EQ(GAME_EVENT_WAIT, out.events[3].kind);
    ASSERT_EQ(GAME_EVENT_CANNOT_MOVE, out.events[4].kind);
    ASSERT_STR_EQ("east", out.events[4].text);
    ASSERT_EQ(GAME_EVENT_UNKNOWN_COMMAND, out.events[5].kind);
    PASS();
}

TEST game_event_push_records_inventory_kinds(void)
{
    GameEventQueue out;

    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_ITEM_RESULT,
        GAME_ITEM_ACTION_TAKE, GAME_ITEM_OUTCOME_OK, 42, 5, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_CORPSE_VIEW, 1, 2, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_BAG_VIEW, 0, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_CRAFT_RESULT,
        ITEM_TORCH, GAME_CRAFT_OUTCOME_OK, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_EQUIP_RESULT,
        ITEM_STICK, GAME_EQUIP_OUTCOME_WIELDED, 0, 0, 0));
    ASSERT_EQ(5, out.count);
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ITEM_ACTION_TAKE, out.events[0].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(42, out.events[0].arg2);
    ASSERT_EQ(5, out.events[0].arg3);
    ASSERT_EQ(GAME_EVENT_CORPSE_VIEW, out.events[1].kind);
    ASSERT_EQ(1, out.events[1].arg0);
    ASSERT_EQ(2, out.events[1].arg1);
    ASSERT_EQ(GAME_EVENT_BAG_VIEW, out.events[2].kind);
    ASSERT_EQ(GAME_EVENT_CRAFT_RESULT, out.events[3].kind);
    ASSERT_EQ(ITEM_TORCH, out.events[3].arg0);
    ASSERT_EQ(GAME_CRAFT_OUTCOME_OK, out.events[3].arg1);
    ASSERT_EQ(GAME_EVENT_EQUIP_RESULT, out.events[4].kind);
    ASSERT_EQ(ITEM_STICK, out.events[4].arg0);
    ASSERT_EQ(GAME_EQUIP_OUTCOME_WIELDED, out.events[4].arg1);
    PASS();
}

TEST game_event_push_records_combat_progression_kinds(void)
{
    GameEventQueue out;

    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_COMBAT,
        GAME_COMBAT_PHASE_START, 10, 20, 3, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_XP_GAIN, 7, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_STAT_CHANGE,
        2, 14, 1, 6, 0));
    ASSERT_EQ(3, out.count);
    ASSERT_EQ(GAME_EVENT_COMBAT, out.events[0].kind);
    ASSERT_EQ(GAME_COMBAT_PHASE_START, out.events[0].arg0);
    ASSERT_EQ(10, out.events[0].arg1);
    ASSERT_EQ(20, out.events[0].arg2);
    ASSERT_EQ(3, out.events[0].arg3);
    ASSERT_EQ(GAME_EVENT_XP_GAIN, out.events[1].kind);
    ASSERT_EQ(7, out.events[1].arg0);
    ASSERT_EQ(GAME_EVENT_STAT_CHANGE, out.events[2].kind);
    ASSERT_EQ(2, out.events[2].arg0);
    ASSERT_EQ(14, out.events[2].arg1);
    PASS();
}

TEST game_event_push_records_watchman_dialogue_scene_arg3(void)
{
    GameEventQueue out;

    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_DIALOGUE,
        GAME_DIALOGUE_ACTOR_WATCHMAN, GAME_DIALOGUE_PHASE_TALK, 0,
        WATCHMAN_SCENE_AFTER_WARNING, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_DIALOGUE,
        GAME_DIALOGUE_ACTOR_WATCHMAN, GAME_DIALOGUE_PHASE_REPLY, 2,
        WATCHMAN_SCENE_FOOD_THANKS, 0));
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(WATCHMAN_SCENE_AFTER_WARNING, out.events[0].arg3);
    ASSERT_EQ(WATCHMAN_SCENE_FOOD_THANKS, out.events[1].arg3);
    PASS();
}

TEST game_event_push_records_ambient_observation_kinds(void)
{
    GameEventQueue out;

    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_ENVIRONMENT,
        GAME_ENV_EVENT_RUSTLE, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_AMBIENT_NOISE,
        0, 0, 0, 0, "crickets"));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_ITEM_PRESENCE,
        ITEM_BERRY, 0, 0, 0, "berry"));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_OBSERVATION,
        GAME_OBS_OUTCOME_WATER, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_ENV_MENU,
        GAME_ENV_WATER, WORLD_ROOM_CAMP, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_ENV_RESULT,
        GAME_ENV_WATER, 1, GAME_ENV_RESULT_DETAIL_NONE, 0, 0));
    ASSERT_EQ(6, out.count);
    ASSERT_EQ(GAME_EVENT_ENVIRONMENT, out.events[0].kind);
    ASSERT_EQ(GAME_ENV_EVENT_RUSTLE, out.events[0].arg0);
    ASSERT_EQ(GAME_EVENT_AMBIENT_NOISE, out.events[1].kind);
    ASSERT_STR_EQ("crickets", out.events[1].text);
    ASSERT_EQ(GAME_EVENT_ITEM_PRESENCE, out.events[2].kind);
    ASSERT_EQ(ITEM_BERRY, out.events[2].arg0);
    ASSERT_EQ(GAME_EVENT_OBSERVATION, out.events[3].kind);
    ASSERT_EQ(GAME_OBS_OUTCOME_WATER, out.events[3].arg0);
    ASSERT_EQ(GAME_EVENT_ENV_MENU, out.events[4].kind);
    ASSERT_EQ(GAME_ENV_WATER, out.events[4].arg0);
    ASSERT_EQ(WORLD_ROOM_CAMP, out.events[4].arg1);
    ASSERT_EQ(GAME_EVENT_ENV_RESULT, out.events[5].kind);
    ASSERT_EQ(GAME_ENV_WATER, out.events[5].arg0);
    ASSERT_EQ(1, out.events[5].arg1);
    PASS();
}

TEST game_event_push_records_weather_environment_kinds(void)
{
    GameEventQueue out;

    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_ENVIRONMENT,
        GAME_ENV_EVENT_WEATHER_RAIN, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_ENVIRONMENT,
        GAME_ENV_EVENT_WEATHER_FOG, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_ENVIRONMENT,
        GAME_ENV_EVENT_WEATHER_WIND, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_ENVIRONMENT,
        GAME_ENV_EVENT_WEATHER_CLEAR, 0, 0, 0, 0));
    ASSERT_EQ(4, out.count);
    ASSERT_EQ(GAME_ENV_EVENT_WEATHER_CLEAR, out.events[3].arg0);
    PASS();
}

TEST game_event_push_records_dialogue_encounter_kinds(void)
{
    GameEventQueue out;

    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_DIALOGUE,
        GAME_DIALOGUE_ACTOR_FROG, GAME_DIALOGUE_PHASE_REPLY, 2, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_ENCOUNTER,
        GAME_ENCOUNTER_BANDIT, GAME_ENCOUNTER_ACTION_GIVE,
        GAME_ENCOUNTER_OUTCOME_OK, ITEM_STICK, "stick"));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_ENCOUNTER,
        GAME_ENCOUNTER_TRAVELER, GAME_ENCOUNTER_ACTION_OPEN,
        GAME_ENCOUNTER_OUTCOME_NONE, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_DIALOGUE_GUARD,
        GAME_DIALOGUE_GUARD_PICK_123, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_DIALOGUE_GUARD,
        GAME_DIALOGUE_GUARD_LOOT_WAITING_REPLY, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_DIALOGUE_GUARD,
        GAME_DIALOGUE_GUARD_DIALOGUE_CLOSED, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_DIALOGUE_GUARD,
        GAME_DIALOGUE_GUARD_GIVE_NO_TARGET, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_DIALOGUE_GUARD,
        GAME_DIALOGUE_GUARD_GIVE_REJECTED, 0, 0, 0, 0));
    ASSERT(0 != game_event_push(&out, GAME_EVENT_DIALOGUE_GUARD,
        GAME_DIALOGUE_GUARD_ENV_MENU_CLOSED, 0, 0, 0, 0));
    ASSERT_EQ(9, out.count);
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_FROG, out.events[0].arg0);
    ASSERT_EQ(2, out.events[0].arg2);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[1].kind);
    ASSERT_EQ(GAME_ENCOUNTER_BANDIT, out.events[1].arg0);
    ASSERT_EQ(ITEM_STICK, out.events[1].arg3);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[2].kind);
    ASSERT_EQ(GAME_ENCOUNTER_TRAVELER, out.events[2].arg0);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_OPEN, out.events[2].arg1);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[3].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_PICK_123, out.events[3].arg0);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[4].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_LOOT_WAITING_REPLY, out.events[4].arg0);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[5].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_DIALOGUE_CLOSED, out.events[5].arg0);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[6].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_GIVE_NO_TARGET, out.events[6].arg0);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[7].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_GIVE_REJECTED, out.events[7].arg0);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[8].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_ENV_MENU_CLOSED, out.events[8].arg0);
    PASS();
}

TEST game_event_push_records_bandit_actor_value(void)
{
    GameEventQueue out;

    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_DIALOGUE,
        GAME_DIALOGUE_ACTOR_BANDIT, GAME_DIALOGUE_PHASE_TALK, 0, 0, 0));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_BANDIT, out.events[0].arg0);
    PASS();
}

TEST game_event_push_records_bandit_ambush_actor_value(void)
{
    GameEventQueue out;

    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_DIALOGUE,
        GAME_DIALOGUE_ACTOR_BANDIT_AMBUSH, GAME_DIALOGUE_PHASE_TALK, 0, 0, 0));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_BANDIT_AMBUSH, out.events[0].arg0);
    PASS();
}

TEST game_dialogue_actor_bandit_ids_are_distinct(void)
{
    /* Authored bandit slots are distinct from the dynamic ambush helper actor. */
    ASSERT(GAME_DIALOGUE_ACTOR_BANDIT != GAME_DIALOGUE_ACTOR_BANDIT_AMBUSH);
    ASSERT(GAME_DIALOGUE_ACTOR_BANDIT_BRIDGE != GAME_DIALOGUE_ACTOR_BANDIT_AMBUSH);
    ASSERT(GAME_DIALOGUE_ACTOR_BANDIT_CANYON != GAME_DIALOGUE_ACTOR_BANDIT_AMBUSH);
    ASSERT_EQ(7, GAME_DIALOGUE_ACTOR_BANDIT);
    ASSERT_EQ(8, GAME_DIALOGUE_ACTOR_BANDIT_BRIDGE);
    ASSERT_EQ(9, GAME_DIALOGUE_ACTOR_BANDIT_CANYON);
    ASSERT_EQ(10, GAME_DIALOGUE_ACTOR_BANDIT_AMBUSH);
    PASS();
}

/*
 * txtres narrative-key lookups live in the gout suite: the contract spans
 * GAME_EVENT_DIALOGUE/ENCOUNTER payload enums and stable txtres scene keys.
 */
TEST txtres_dialogue_narrative_key_maps_stable_scenes(void)
{
    ASSERT_EQ(TXTRES_NARRATIVE_FROG_TALK,
        txtres_dialogue_narrative_key(GAME_DIALOGUE_ACTOR_FROG,
            GAME_DIALOGUE_PHASE_TALK));
    ASSERT_EQ(TXTRES_NARRATIVE_FROG_REPLY,
        txtres_dialogue_narrative_key(GAME_DIALOGUE_ACTOR_FROG,
            GAME_DIALOGUE_PHASE_REPLY));
    ASSERT_EQ(TXTRES_NARRATIVE_WATCHMAN_TALK,
        txtres_dialogue_narrative_key(GAME_DIALOGUE_ACTOR_WATCHMAN,
            GAME_DIALOGUE_PHASE_TALK));
    ASSERT_EQ(TXTRES_NARRATIVE_HERBALIST_TALK,
        txtres_dialogue_narrative_key(GAME_DIALOGUE_ACTOR_HERBALIST,
            GAME_DIALOGUE_PHASE_TALK));
    ASSERT_EQ(TXTRES_NARRATIVE_HERBALIST_REPLY,
        txtres_dialogue_narrative_key(GAME_DIALOGUE_ACTOR_HERBALIST,
            GAME_DIALOGUE_PHASE_REPLY));
    ASSERT_EQ(TXTRES_NARRATIVE_TRAVELER_REPLY,
        txtres_dialogue_narrative_key(GAME_DIALOGUE_ACTOR_TRAVELER,
            GAME_DIALOGUE_PHASE_REPLY));
    ASSERT_EQ(TXTRES_NARRATIVE_LOST_ANIMAL_REPLY,
        txtres_dialogue_narrative_key(GAME_DIALOGUE_ACTOR_LOST_ANIMAL,
            GAME_DIALOGUE_PHASE_REPLY));
    ASSERT_EQ(TXTRES_NARRATIVE_PEDDLER_REPLY,
        txtres_dialogue_narrative_key(GAME_DIALOGUE_ACTOR_PEDDLER,
            GAME_DIALOGUE_PHASE_REPLY));
    ASSERT_EQ(TXTRES_NARRATIVE_ARCHIVIST_REPLY,
        txtres_dialogue_narrative_key(GAME_DIALOGUE_ACTOR_ARCHIVIST,
            GAME_DIALOGUE_PHASE_REPLY));
    ASSERT_EQ(TXTRES_NARRATIVE_NONE,
        txtres_dialogue_narrative_key(GAME_DIALOGUE_ACTOR_TRAVELER,
            GAME_DIALOGUE_PHASE_TALK));
    ASSERT_EQ(TXTRES_NARRATIVE_NONE,
        txtres_dialogue_narrative_key(99, GAME_DIALOGUE_PHASE_TALK));
    PASS();
}

TEST txtres_encounter_narrative_key_maps_stable_scenes(void)
{
    ASSERT_EQ(TXTRES_NARRATIVE_BANDIT_OPEN,
        txtres_encounter_narrative_key(GAME_ENCOUNTER_BANDIT,
            GAME_ENCOUNTER_ACTION_OPEN, GAME_ENCOUNTER_OUTCOME_NONE));
    ASSERT_EQ(TXTRES_NARRATIVE_TRAVELER_SCENE,
        txtres_encounter_narrative_key(GAME_ENCOUNTER_TRAVELER,
            GAME_ENCOUNTER_ACTION_OPEN, GAME_ENCOUNTER_OUTCOME_NONE));
    ASSERT_EQ(TXTRES_NARRATIVE_LOST_ANIMAL_SCENE,
        txtres_encounter_narrative_key(GAME_ENCOUNTER_LOST_ANIMAL,
            GAME_ENCOUNTER_ACTION_OPEN, GAME_ENCOUNTER_OUTCOME_NONE));
    ASSERT_EQ(TXTRES_NARRATIVE_PEDDLER_SCENE,
        txtres_encounter_narrative_key(GAME_ENCOUNTER_PEDDLER,
            GAME_ENCOUNTER_ACTION_OPEN, GAME_ENCOUNTER_OUTCOME_NONE));
    ASSERT_EQ(TXTRES_NARRATIVE_BANDIT_GIVE_OK,
        txtres_encounter_narrative_key(GAME_ENCOUNTER_BANDIT,
            GAME_ENCOUNTER_ACTION_GIVE, GAME_ENCOUNTER_OUTCOME_OK));
    ASSERT_EQ(TXTRES_NARRATIVE_BANDIT_INTIMIDATE_FAIL,
        txtres_encounter_narrative_key(GAME_ENCOUNTER_BANDIT,
            GAME_ENCOUNTER_ACTION_INTIMIDATE,
            GAME_ENCOUNTER_OUTCOME_FAIL));
    ASSERT_EQ(TXTRES_NARRATIVE_NONE,
        txtres_encounter_narrative_key(GAME_ENCOUNTER_TRAVELER,
            GAME_ENCOUNTER_ACTION_GIVE, GAME_ENCOUNTER_OUTCOME_OK));
    PASS();
}

SUITE(gout) {
    RUN_TEST(game_event_queue_reset_clears_state);
    RUN_TEST(game_event_push_ignores_null_output);
    RUN_TEST(game_event_push_marks_overflow_at_capacity);
    RUN_TEST(game_event_queue_reset_clears_overflow_and_reuse_state);
    RUN_TEST(game_event_push_records_generic_move);
    RUN_TEST(game_event_push_preserves_enqueue_order_and_payload_slots);
    RUN_TEST(game_event_push_records_command_nav_kinds);
    RUN_TEST(game_event_push_records_inventory_kinds);
    RUN_TEST(game_event_push_records_combat_progression_kinds);
    RUN_TEST(game_event_push_records_ambient_observation_kinds);
    RUN_TEST(game_event_push_records_weather_environment_kinds);
    RUN_TEST(game_event_push_records_dialogue_encounter_kinds);
    RUN_TEST(game_event_push_records_watchman_dialogue_scene_arg3);
    RUN_TEST(game_event_push_records_bandit_actor_value);
    RUN_TEST(game_event_push_records_bandit_ambush_actor_value);
    RUN_TEST(game_dialogue_actor_bandit_ids_are_distinct);
    RUN_TEST(txtres_dialogue_narrative_key_maps_stable_scenes);
    RUN_TEST(txtres_encounter_narrative_key_maps_stable_scenes);
}
