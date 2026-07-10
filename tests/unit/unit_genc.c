#include "greatest.h"
#include "config.h"
#include "game.h"
#include "genc.h"
#include "gout.h"
#include "invent.h"
#include "items.h"
#include "npc.h"
#include "unit_util.h"

/*
 * Direct genc API tests with local GameEventQueue fixtures.
 * out-taking helpers assert #160 encounter events; *_state drops the queue.
 * begin_enemy_kind sets encounter ids the normal open path never assigns.
 */
static void begin_enemy(struct GameState *game, GameEventQueue *out)
{
    game_event_queue_reset(out);
    enemy_begin_encounter(game, out);
}

static int enemy_reply(struct GameState *game, int choice, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return genc_cmd_reply(game, choice, out);
}

static int enemy_give(struct GameState *game, int item_id, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return genc_cmd_give(game, item_id, out);
}

static struct NpcState *bandit_npc(struct GameState *game)
{
    int slot;

    slot = npc_find_by_actor(game, GAME_DIALOGUE_ACTOR_BANDIT);
    if (slot < 0) {
        return 0;
    }
    return &game->npcs[slot];
}

static struct NpcState *active_enemy_npc(struct GameState *game)
{
    int slot;

    slot = npc_find_by_dialogue(game, DIALOGUE_ENEMY);
    if (slot < 0) {
        return 0;
    }
    return &game->npcs[slot];
}

static void begin_enemy_state(struct GameState *game)
{
    GameEventQueue out;

    begin_enemy(game, &out);
}

/* Stub encounter id on an active enemy slot; skips enemy_begin_encounter ordering. */
static int begin_enemy_kind(struct GameState *game, int actor, int encounter,
                            GameEventQueue *out)
{
    game_event_queue_reset(out);
    return npc_begin_encounter(game, actor, DIALOGUE_ENEMY, encounter,
        game->player.room_id, 0, out);
}

TEST genc_skips_when_busy(void)
{
    struct GameState game;

    unit_game_fresh(&game, 1u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    begin_enemy_state(&game);
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    PASS();
}

TEST genc_opens_dialogue(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *enemy;

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy(&game, &out);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_ENEMY, game.dialogue);
    enemy = active_enemy_npc(&game);
    ASSERT(enemy != 0);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_BANDIT, out.events[0].arg0);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_OPEN, out.events[0].arg1);
    ASSERT_EQ(enemy->level, out.events[0].arg3);
    PASS();
}

TEST genc_opens_seeded_roaming_bandit_without_spawning_ambush(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *bandit;

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ROAD, 0);
    bandit = bandit_npc(&game);
    ASSERT(bandit != 0);
    begin_enemy(&game, &out);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_ENEMY, game.dialogue);
    ASSERT_EQ(WORLD_ROOM_ROAD, bandit->room_id);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_BANDIT, bandit->actor);
    ASSERT_EQ(NPC_FLAG_ACTIVE, bandit->flags & NPC_FLAG_ACTIVE);
    ASSERT_EQ(NPC_FLAG_ROAMING, bandit->flags & NPC_FLAG_ROAMING);
    ASSERT_EQ(NPC_FLAG_RESPAWNS, bandit->flags & NPC_FLAG_RESPAWNS);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_OPEN, out.events[0].arg1);
    ASSERT_EQ(bandit->level, out.events[0].arg3);
    PASS();
}

TEST genc_random_ambush_keeps_roster_bandit_seeded(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *bandit;
    int slot;

    unit_game_fresh(&game, 22u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    bandit = bandit_npc(&game);
    ASSERT(bandit != 0);
    ASSERT_EQ(WORLD_ROOM_ROAD, bandit->room_id);
    begin_enemy(&game, &out);
    slot = npc_find_by_dialogue(&game, DIALOGUE_ENEMY);
    ASSERT(slot >= 0);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_BANDIT_AMBUSH, game.npcs[slot].actor);
    ASSERT_EQ(WORLD_ROOM_CAMP, game.npcs[slot].room_id);
    ASSERT_EQ(WORLD_ROOM_ROAD, bandit->room_id);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_BANDIT, bandit->actor);
    PASS();
}

TEST genc_cmd_reply_fight(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 3u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy_state(&game);
    ASSERT_EQ(1, enemy_reply(&game, 1, &out));
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    ASSERT_EQ(4, out.count);
    ASSERT_EQ(GAME_EVENT_COMBAT, out.events[0].kind);
    ASSERT_EQ(GAME_COMBAT_PHASE_START, out.events[0].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_PLAYER_DAMAGE, out.events[1].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_STATUS, out.events[2].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_MENU, out.events[3].arg0);
    PASS();
}

TEST genc_cmd_reply_intimidate_ok(void)
{
    struct GameState game;
    GameEventQueue out;
    int rolls[1];

    unit_game_fresh(&game, 4u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy_state(&game);
    rolls[0] = CFG_TEST_INTIMIDATE_OK;
    game_roll_inject_begin(&game, rolls, 1);
    ASSERT_EQ(1, enemy_reply(&game, 3, &out));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_INTIMIDATE, out.events[0].arg1);
    ASSERT_EQ(GAME_ENCOUNTER_OUTCOME_SUCCESS, out.events[0].arg2);
    ASSERT_EQ(GAME_EVENT_ROOM_LOOK, out.events[1].kind);
    ASSERT_EQ(GAME_ROOM_LOOK_FLAG_TIGHT_LEAD,
        out.events[1].arg3 & GAME_ROOM_LOOK_FLAG_TIGHT_LEAD);
    PASS();
}

TEST genc_cmd_give_wrong_context(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 5u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, enemy_give(&game, ITEM_STICK, &out));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_OUTCOME_WRONG_CONTEXT, out.events[0].arg2);
    PASS();
}

TEST genc_cmd_give_handover(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *enemy;

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy_state(&game);
    game_inv_bag_add(&game, ITEM_STICK);
    enemy = active_enemy_npc(&game);
    ASSERT(enemy != 0);
    enemy->flags |= NPC_FLAG_HANDOVER_PICK;
    ASSERT_EQ(1, enemy_give(&game, ITEM_STICK, &out));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_GIVE, out.events[0].arg1);
    ASSERT_EQ(GAME_ENCOUNTER_OUTCOME_OK, out.events[0].arg2);
    ASSERT_EQ(ITEM_STICK, out.events[0].arg3);
    ASSERT_EQ(GAME_EVENT_ROOM_LOOK, out.events[1].kind);
    ASSERT_EQ(GAME_ROOM_LOOK_FLAG_TIGHT_LEAD,
        out.events[1].arg3 & GAME_ROOM_LOOK_FLAG_TIGHT_LEAD);
    PASS();
}

TEST genc_cmd_reply_handover_pick(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *enemy;

    unit_game_fresh(&game, 7u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy_state(&game);
    game_inv_bag_add(&game, ITEM_STICK);
    ASSERT_EQ(1, enemy_reply(&game, 2, &out));
    enemy = active_enemy_npc(&game);
    ASSERT(enemy != 0);
    /* handover gating reads NPC_FLAG_HANDOVER_PICK on the enemy slot */
    ASSERT_EQ(NPC_FLAG_HANDOVER_PICK,
        enemy->flags & NPC_FLAG_HANDOVER_PICK);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_HANDOVER_PROMPT, out.events[0].arg1);
    PASS();
}

TEST genc_replay_active_prompt_reuses_open_or_handover_copy(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *enemy;

    unit_game_fresh(&game, 70u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy_state(&game);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, genc_replay_active_prompt(&game, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_OPEN, out.events[0].arg1);

    enemy = active_enemy_npc(&game);
    ASSERT(enemy != 0);
    enemy->flags |= NPC_FLAG_HANDOVER_PICK;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, genc_replay_active_prompt(&game, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_HANDOVER_PROMPT, out.events[0].arg1);
    PASS();
}

TEST genc_cmd_reply_intimidate_clears_handover_pick(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *enemy;
    int rolls[1];

    unit_game_fresh(&game, 9u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy_state(&game);
    game_inv_bag_add(&game, ITEM_STICK);
    ASSERT_EQ(1, enemy_reply(&game, 2, &out));
    enemy = active_enemy_npc(&game);
    ASSERT(enemy != 0);
    ASSERT_EQ(NPC_FLAG_HANDOVER_PICK,
        enemy->flags & NPC_FLAG_HANDOVER_PICK);
    rolls[0] = CFG_TEST_INTIMIDATE_FAIL;
    game_roll_inject_begin(&game, rolls, 1);
    ASSERT_EQ(1, enemy_reply(&game, 3, &out));
    ASSERT_EQ(0, enemy->flags & NPC_FLAG_HANDOVER_PICK);
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    PASS();
}

TEST genc_cmd_reply_intimidate_fail(void)
{
    struct GameState game;
    GameEventQueue out;
    int rolls[1];

    unit_game_fresh(&game, 8u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy_state(&game);
    rolls[0] = CFG_TEST_INTIMIDATE_FAIL;
    game_roll_inject_begin(&game, rolls, 1);
    ASSERT_EQ(1, enemy_reply(&game, 3, &out));
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    ASSERT_EQ(5, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_OUTCOME_FAIL, out.events[0].arg2);
    ASSERT_EQ(GAME_EVENT_COMBAT, out.events[1].kind);
    ASSERT_EQ(GAME_COMBAT_PHASE_START, out.events[1].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_ENEMY_DAMAGE, out.events[2].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_STATUS, out.events[3].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_MENU, out.events[4].arg0);
    PASS();
}

TEST genc_cmd_reply_invalid_choice(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 9u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy_state(&game);
    ASSERT_EQ(1, enemy_reply(&game, 0, &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_PICK_123, out.events[0].arg0);
    PASS();
}

/* Friendly roaming kinds have empty genc rows; reply returns 0 with an empty queue. */
TEST genc_cmd_reply_lost_animal_encounter_kind_unhandled(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 10u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT(begin_enemy_kind(&game, GAME_DIALOGUE_ACTOR_BANDIT_AMBUSH,
        GAME_ENCOUNTER_LOST_ANIMAL, &out) >= 0);
    ASSERT_EQ(0, enemy_reply(&game, 1, &out));
    ASSERT_EQ(0, out.count);
    PASS();
}

TEST genc_cmd_reply_peddler_encounter_kind_unhandled(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 11u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT(begin_enemy_kind(&game, GAME_DIALOGUE_ACTOR_BANDIT_AMBUSH,
        GAME_ENCOUNTER_PEDDLER, &out) >= 0);
    ASSERT_EQ(0, enemy_reply(&game, 2, &out));
    ASSERT_EQ(0, out.count);
    PASS();
}

/* GAME_ENCOUNTER_TRAVELER has no genc row; reply returns 0 with an empty queue. */
TEST genc_cmd_reply_unsupported_encounter_kind(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 10u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT(begin_enemy_kind(&game, GAME_DIALOGUE_ACTOR_BANDIT_AMBUSH,
        GAME_ENCOUNTER_TRAVELER, &out) >= 0);
    ASSERT_EQ(0, enemy_reply(&game, 1, &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(0, out.count);
    PASS();
}

TEST genc_cmd_reply_bag_empty_then_combat(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 10u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy_state(&game);
    ASSERT_EQ(1, enemy_reply(&game, 2, &out));
    ASSERT_EQ(5, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_HANDOVER, out.events[0].arg1);
    ASSERT_EQ(GAME_ENCOUNTER_OUTCOME_BAG_EMPTY, out.events[0].arg2);
    ASSERT_EQ(GAME_EVENT_COMBAT, out.events[1].kind);
    ASSERT_EQ(GAME_COMBAT_PHASE_START, out.events[1].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_ENEMY_DAMAGE, out.events[2].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_STATUS, out.events[3].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_MENU, out.events[4].arg0);
    PASS();
}

/* Unsupported kind still emits bandit GIVE WRONG_CONTEXT (always-consumed give path). */
TEST genc_cmd_give_unsupported_encounter_kind_falls_back_to_wrong_context(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 11u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT(begin_enemy_kind(&game, GAME_DIALOGUE_ACTOR_BANDIT_AMBUSH,
        GAME_ENCOUNTER_TRAVELER, &out) >= 0);
    ASSERT_EQ(1, enemy_give(&game, ITEM_STICK, &out));
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_BANDIT, out.events[0].arg0);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_GIVE, out.events[0].arg1);
    ASSERT_EQ(GAME_ENCOUNTER_OUTCOME_WRONG_CONTEXT, out.events[0].arg2);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[1].kind);
    ASSERT_EQ(GAME_ENCOUNTER_TRAVELER, out.events[1].arg0);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_OPEN, out.events[1].arg1);
    PASS();
}

SUITE(genc) {
    RUN_TEST(genc_skips_when_busy);
    RUN_TEST(genc_opens_dialogue);
    RUN_TEST(genc_opens_seeded_roaming_bandit_without_spawning_ambush);
    RUN_TEST(genc_random_ambush_keeps_roster_bandit_seeded);
    RUN_TEST(genc_cmd_reply_fight);
    RUN_TEST(genc_cmd_reply_intimidate_ok);
    RUN_TEST(genc_cmd_give_wrong_context);
    RUN_TEST(genc_cmd_give_handover);
    RUN_TEST(genc_cmd_reply_handover_pick);
    RUN_TEST(genc_replay_active_prompt_reuses_open_or_handover_copy);
    RUN_TEST(genc_cmd_reply_intimidate_clears_handover_pick);
    RUN_TEST(genc_cmd_reply_intimidate_fail);
    RUN_TEST(genc_cmd_reply_invalid_choice);
    RUN_TEST(genc_cmd_reply_lost_animal_encounter_kind_unhandled);
    RUN_TEST(genc_cmd_reply_peddler_encounter_kind_unhandled);
    RUN_TEST(genc_cmd_reply_unsupported_encounter_kind);
    RUN_TEST(genc_cmd_reply_bag_empty_then_combat);
    RUN_TEST(genc_cmd_give_unsupported_encounter_kind_falls_back_to_wrong_context);
}
