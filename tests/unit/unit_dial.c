#include "greatest.h"
#include "config.h"
#include "dialogue.h"
#include "game.h"
#include "gout.h"
#include "invent.h"
#include "items.h"
#include "npc.h"
#include "txtres.h"
#include "world.h"
#include "unit_util.h"

/*
 * Dialogue router tests: #160 GameEvent payloads and mode transitions.
 * npc.c lookup tables and direct open/talk seams live in unit_npc.c.
 */
static int talk_out(struct GameState *game, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return dialogue_cmd_talk(game, out);
}

static int reply_out(struct GameState *game, int choice, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return dialogue_cmd_reply(game, choice, out);
}

static int talk_out_state(struct GameState *game)
{
    GameEventQueue out;

    return talk_out(game, &out);
}

static int reply_out_state(struct GameState *game, int choice)
{
    GameEventQueue out;

    return reply_out(game, choice, &out);
}

static int room_has_item(const struct GameState *game, int room_id, int item_id)
{
    int slot;

    for (slot = 0; slot < CFG_AREA_ITEM_SLOTS; ++slot) {
        if (game->room_item[room_id][slot] == item_id) {
            return 1;
        }
    }
    return 0;
}

TEST dialogue_cmd_talk_watchman(void)
{
    struct GameState game;

    unit_game_fresh(&game, 4u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    ASSERT_EQ(1, talk_out_state(&game));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_WATCHMAN, game.dialogue);
    PASS();
}

TEST dialogue_cmd_talk_nobody_camp(void)
{
    struct GameState game;

    unit_game_fresh(&game, 5u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, talk_out_state(&game));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST dialogue_cmd_reply_frog(void)
{
    struct GameState game;

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    talk_out_state(&game);
    ASSERT_EQ(1, reply_out_state(&game, 2));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST dialogue_cmd_reply_not_dialogue(void)
{
    struct GameState game;

    unit_game_fresh(&game, 7u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(0, reply_out_state(&game, 1));
    PASS();
}

TEST dialogue_cmd_talk_frog(void)
{
    struct GameState game;

    unit_game_fresh(&game, 8u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    ASSERT_EQ(1, talk_out_state(&game));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_FROG, game.dialogue);
    PASS();
}

TEST dialogue_cmd_talk_bandit_blocks(void)
{
    struct GameState game;

    unit_game_fresh(&game, 9u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_ENEMY);
    ASSERT_EQ(1, talk_out_state(&game));
    ASSERT_EQ(DIALOGUE_ENEMY, game.dialogue);
    PASS();
}

TEST dialogue_cmd_talk_traveler_waiting(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 10u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_TRAVELER);
    ASSERT_EQ(1, talk_out(&game, &out));
    ASSERT_EQ(DIALOGUE_TRAVELER, game.dialogue);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_TRAVELER_WAITING, out.events[0].arg0);
    PASS();
}

TEST dialogue_cmd_talk_herbalist(void)
{
    struct GameState game;

    unit_game_fresh(&game, 11u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    ASSERT_EQ(1, talk_out_state(&game));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_HERBALIST, game.dialogue);
    PASS();
}

TEST dialogue_cmd_talk_archivist(void)
{
    struct GameState game;

    unit_game_fresh(&game, 12u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CATACOMBS, 0);
    ASSERT_EQ(1, talk_out_state(&game));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_ARCHIVIST, game.dialogue);
    PASS();
}

TEST dialogue_cmd_reply_frog_invalid(void)
{
    struct GameState game;

    unit_game_fresh(&game, 13u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    talk_out_state(&game);
    ASSERT_EQ(1, reply_out_state(&game, 0));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_FROG, game.dialogue);
    PASS();
}

TEST dialogue_cmd_reply_watchman(void)
{
    struct GameState game;

    unit_game_fresh(&game, 14u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    talk_out_state(&game);
    ASSERT_EQ(1, reply_out_state(&game, 1));
    ASSERT_EQ(WATCHMAN_FLAG_WARNED, game.watchman_flags);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(WATCHMAN_SCENE_AFTER_WARNING, game.watchman_menu);
    PASS();
}

TEST dialogue_cmd_reply_herbalist(void)
{
    struct GameState game;

    unit_game_fresh(&game, 15u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    talk_out_state(&game);
    ASSERT_EQ(1, reply_out_state(&game, 2));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST dialogue_cmd_reply_herbalist_starts_request_and_seeds_root(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 115u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    ASSERT_EQ(1, talk_out(&game, &out));
    ASSERT_EQ(HERBALIST_SCENE_NOT_STARTED, out.events[0].arg3);
    ASSERT_EQ(1, reply_out(&game, 1, &out));
    ASSERT_EQ(HERBALIST_STORY_REQUESTED, game.herbalist_story);
    ASSERT_EQ(1, game.marsh_root_spawned);
    ASSERT(room_has_item(&game, WORLD_ROOM_MARSH, ITEM_MARSH_ROOT));
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(HERBALIST_SCENE_NOT_STARTED, out.events[0].arg3);
    PASS();
}

TEST dialogue_cmd_talk_herbalist_ready_scene(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 116u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_REQUESTED;
    game.marsh_root_spawned = 1;
    game_inv_bag_add(&game, ITEM_MARSH_ROOT);
    ASSERT_EQ(1, talk_out(&game, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(HERBALIST_SCENE_READY, out.events[0].arg3);
    PASS();
}

TEST dialogue_cmd_reply_herbalist_turn_in_completes_story(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 117u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_REQUESTED;
    game.marsh_root_spawned = 1;
    game_inv_bag_add(&game, ITEM_MARSH_ROOT);
    ASSERT_EQ(1, talk_out(&game, &out));
    ASSERT_EQ(1, reply_out(&game, 1, &out));
    ASSERT_EQ(HERBALIST_STORY_COMPLETE, game.herbalist_story);
    ASSERT_EQ(-1, game_inv_bag_find_index(&game, ITEM_MARSH_ROOT));
    ASSERT_STR_EQ(TXT_STORY_ORCHARD_DONE_DESC,
        game.world.rooms[WORLD_ROOM_ORCHARD].desc);
    ASSERT_EQ(HERBALIST_SCENE_GIVE_REWARD_BAG, out.events[0].arg3);
    PASS();
}

TEST dialogue_cmd_talk_herbalist_complete_scene(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 118u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_COMPLETE;
    ASSERT_EQ(1, talk_out(&game, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(HERBALIST_SCENE_COMPLETE, out.events[0].arg3);
    PASS();
}

TEST dialogue_cmd_reply_archivist(void)
{
    struct GameState game;

    unit_game_fresh(&game, 16u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CATACOMBS, 0);
    talk_out_state(&game);
    ASSERT_EQ(1, reply_out_state(&game, 3));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST dialogue_cmd_talk_watchman_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 20u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    ASSERT_EQ(1, talk_out(&game, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_WATCHMAN, out.events[0].arg0);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_TALK, out.events[0].arg1);
    ASSERT_EQ(WATCHMAN_SCENE_NEUTRAL, out.events[0].arg3);
    PASS();
}

/*
 * Warning reply chains REPLY then TALK in one turn; no second talk required.
 */
TEST dialogue_cmd_reply_watchman_sets_warned_and_followup_talk(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 141u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    ASSERT_EQ(1, talk_out(&game, &out));
    ASSERT_EQ(1, reply_out(&game, 1, &out));
    ASSERT_EQ(WATCHMAN_FLAG_WARNED, game.watchman_flags);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(WATCHMAN_SCENE_AFTER_WARNING, game.watchman_menu);
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_REPLY, out.events[0].arg1);
    ASSERT_EQ(WATCHMAN_SCENE_NEUTRAL, out.events[0].arg3);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_TALK, out.events[1].arg1);
    ASSERT_EQ(WATCHMAN_SCENE_AFTER_WARNING, out.events[1].arg3);
    PASS();
}

TEST dialogue_cmd_reply_watchman_meal_thread_grants_herb(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 142u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    game_inv_bag_add(&game, ITEM_BERRY);
    talk_out(&game, &out);
    ASSERT_EQ(1, reply_out(&game, 2, &out));
    ASSERT_EQ(WATCHMAN_SCENE_MEAL_OFFER, game.watchman_menu);
    ASSERT_EQ(WATCHMAN_SCENE_PECKISH, out.events[0].arg3);
    ASSERT_EQ(1, reply_out(&game, 1, &out));
    ASSERT_EQ(WATCHMAN_FLAG_HERBS, game.watchman_flags);
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_HERB));
    ASSERT_EQ(0, game_inv_player_has_item(&game, ITEM_BERRY));
    PASS();
}

TEST dialogue_cmd_reply_watchman_warned_and_herbs_flags_coexist(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 145u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    game.watchman_flags = WATCHMAN_FLAG_WARNED | WATCHMAN_FLAG_HERBS;
    talk_out(&game, &out);
    ASSERT_EQ(WATCHMAN_SCENE_NEUTRAL, out.events[0].arg3);
    PASS();
}

TEST dialogue_cmd_reply_watchman_invalid_reply_preserves_state(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 143u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    talk_out(&game, &out);
    ASSERT_EQ(1, reply_out(&game, 4, &out));
    ASSERT_EQ(0, game.watchman_flags);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_PICK_123, out.events[0].arg0);
    PASS();
}

TEST dialogue_cmd_reply_watchman_herbs_already_given(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 144u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    game.watchman_flags = WATCHMAN_FLAG_HERBS;
    talk_out(&game, &out);
    ASSERT_EQ(1, reply_out(&game, 2, &out));
    ASSERT_EQ(WATCHMAN_SCENE_ALREADY_FED, out.events[0].arg3);
    ASSERT_EQ(WATCHMAN_SCENE_AFTER_MEAL, game.watchman_menu);
    PASS();
}

TEST dialogue_cmd_talk_frog_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 24u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    ASSERT_EQ(1, talk_out(&game, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_FROG, out.events[0].arg0);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_TALK, out.events[0].arg1);
    PASS();
}

TEST dialogue_cmd_reply_frog_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 25u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    talk_out(&game, &out);
    ASSERT_EQ(1, reply_out(&game, 2, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_FROG, out.events[0].arg0);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_REPLY, out.events[0].arg1);
    ASSERT_EQ(2, out.events[0].arg2);
    PASS();
}

/* Reply follows game.dialogue, not player room, after a mid-branch move. */
TEST dialogue_cmd_reply_frog_after_move_stays_on_dialogue_table(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 26u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    talk_out(&game, &out);
    game.player.room_id = WORLD_ROOM_CAMP;
    ASSERT_EQ(1, reply_out(&game, 1, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_FROG, out.events[0].arg0);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_REPLY, out.events[0].arg1);
    ASSERT_EQ(1, out.events[0].arg2);
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST dialogue_cmd_talk_nobody_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 21u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, talk_out(&game, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_NOBODY, out.events[0].arg0);
    PASS();
}

TEST dialogue_cmd_talk_bandit_blocks_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 22u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_ENEMY);
    ASSERT_EQ(1, talk_out(&game, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_BANDIT_BLOCKS_TALK, out.events[0].arg0);
    PASS();
}

TEST dialogue_cmd_reply_frog_invalid_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 23u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    talk_out(&game, &out);
    ASSERT_EQ(1, reply_out(&game, 0, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_PICK_123, out.events[0].arg0);
    PASS();
}

SUITE(dialogue) {
    RUN_TEST(dialogue_cmd_talk_watchman);
    RUN_TEST(dialogue_cmd_talk_nobody_camp);
    RUN_TEST(dialogue_cmd_reply_frog);
    RUN_TEST(dialogue_cmd_reply_not_dialogue);
    RUN_TEST(dialogue_cmd_talk_frog);
    RUN_TEST(dialogue_cmd_talk_bandit_blocks);
    RUN_TEST(dialogue_cmd_talk_traveler_waiting);
    RUN_TEST(dialogue_cmd_talk_herbalist);
    RUN_TEST(dialogue_cmd_talk_archivist);
    RUN_TEST(dialogue_cmd_reply_frog_invalid);
    RUN_TEST(dialogue_cmd_reply_watchman);
    RUN_TEST(dialogue_cmd_reply_watchman_sets_warned_and_followup_talk);
    RUN_TEST(dialogue_cmd_reply_watchman_meal_thread_grants_herb);
    RUN_TEST(dialogue_cmd_reply_watchman_warned_and_herbs_flags_coexist);
    RUN_TEST(dialogue_cmd_reply_watchman_invalid_reply_preserves_state);
    RUN_TEST(dialogue_cmd_reply_watchman_herbs_already_given);
    RUN_TEST(dialogue_cmd_reply_herbalist);
    RUN_TEST(dialogue_cmd_reply_herbalist_starts_request_and_seeds_root);
    RUN_TEST(dialogue_cmd_talk_herbalist_ready_scene);
    RUN_TEST(dialogue_cmd_reply_herbalist_turn_in_completes_story);
    RUN_TEST(dialogue_cmd_talk_herbalist_complete_scene);
    RUN_TEST(dialogue_cmd_reply_archivist);
    RUN_TEST(dialogue_cmd_talk_watchman_event);
    RUN_TEST(dialogue_cmd_talk_frog_event);
    RUN_TEST(dialogue_cmd_reply_frog_event);
    RUN_TEST(dialogue_cmd_reply_frog_after_move_stays_on_dialogue_table);
    RUN_TEST(dialogue_cmd_talk_nobody_event);
    RUN_TEST(dialogue_cmd_talk_bandit_blocks_event);
    RUN_TEST(dialogue_cmd_reply_frog_invalid_event);
}
