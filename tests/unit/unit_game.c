#include <string.h>
#include "greatest.h"
#include "config.h"
#include "game.h"
#include "gatmos.h"
#include "genc.h"
#include "grendr.h"
#include "gwhok.h"
#include "invent.h"
#include "items.h"
#include "npc.h"
#include "platform.h"
#include "testharn.h"
#include "txtres.h"
#include "unit_util.h"

static int run_cmd_out(struct GameState *game, const char *line,
                       GameEventQueue *out)
{
    char buf[CFG_INPUT_MAX];

    game_event_queue_reset(out);
    strncpy(buf, line, CFG_INPUT_MAX - 1);
    buf[CFG_INPUT_MAX - 1] = '\0';
    return game_process_input(game, buf, out);
}

static int run_cmd(struct GameState *game, const char *line)
{
    GameEventQueue out;

    return run_cmd_out(game, line, &out);
}

/* Same deferral as fixture_traveler_off; keeps quiet-tick tests deterministic. */
static void disable_traveler(struct GameState *game)
{
    npc_deactivate_until(game, GAME_DIALOGUE_ACTOR_TRAVELER, 999999UL);
}

static struct NpcState *traveler_npc(struct GameState *game)
{
    int slot;

    slot = npc_find_by_actor(game, GAME_DIALOGUE_ACTOR_TRAVELER);
    if (slot < 0) {
        return 0;
    }
    return &game->npcs[slot];
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
    game.combat.enemy_hp = 6;
    game.combat.enemy_level = 2;
    game.combat.defending = 1;
    game_set_mode_dialogue(&game, DIALOGUE_NPC_FROG);
    ASSERT_EQ(1, game_is_busy_dialogue(&game));
    game_set_mode_combat(&game);
    ASSERT_EQ(1, game_is_busy_dialogue(&game));
    game_set_mode_explore(&game);
    ASSERT_EQ(0, game_is_busy_dialogue(&game));
    ASSERT_EQ(0, game.combat.enemy_hp);
    ASSERT_EQ(0, game.combat.enemy_level);
    ASSERT_EQ(0, game.combat.defending);
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
    disable_traveler(&game);
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
    GameEventQueue out;

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.env_room_clues[WORLD_ROOM_CAMP] |= (u8)(1u << (GAME_ENV_WATER - 1));
    ASSERT_EQ(1, run_cmd_out(&game, "inspect water", &out));
    ASSERT_EQ(0, game.env_room_clues[WORLD_ROOM_CAMP] &
        (u8)(1u << (GAME_ENV_WATER - 1)));
    ASSERT_EQ(1, game.env_interact_active);
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_OBSERVATION, out.events[0].kind);
    ASSERT_EQ(GAME_OBS_OUTCOME_WATER, out.events[0].arg0);
    ASSERT_EQ(GAME_EVENT_ENV_MENU, out.events[1].kind);
    PASS();
}

TEST game_env_inspect_reply(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.env_room_clues[WORLD_ROOM_CAMP] |= (u8)(1u << (GAME_ENV_WATER - 1));
    ASSERT_EQ(1, run_cmd_out(&game, "inspect", &out));
    ASSERT_EQ(1, game.env_interact_active);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, run_cmd_out(&game, "1", &out));
    ASSERT_EQ(0, game.env_interact_active);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ENV_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ENV_WATER, out.events[0].arg0);
    ASSERT_EQ(1, out.events[0].arg1);
    PASS();
}

TEST game_env_menu_dismiss_on_move(void)
{
    /* game.c maybe_dismiss_env_menu clears gatmos state before move applies. */
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.env_interact_active = 1;
    game.env_interact_kind = GAME_ENV_WATER;
    game.env_interact_room = WORLD_ROOM_CAMP;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, run_cmd_out(&game, "north", &out));
    ASSERT_EQ(0, game.env_interact_active);
    PASS();
}

TEST game_env_reply_room_mismatch_fail_closed(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ROAD, 0);
    game.env_interact_active = 1;
    game.env_interact_kind = GAME_ENV_WATER;
    game.env_interact_room = WORLD_ROOM_CAMP;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, run_cmd_out(&game, "1", &out));
    ASSERT_EQ(0, game.env_interact_active);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_ENV_MENU_CLOSED, out.events[0].arg0);
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
    GameEventQueue out;

    unit_game_fresh(&game, 10u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_event_queue_reset(&out);
    enemy_begin_encounter(&game, &out);
    ASSERT_EQ(1, run_cmd_out(&game, "1", &out));
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    ASSERT_EQ(4, out.count);
    ASSERT_EQ(GAME_EVENT_COMBAT, out.events[0].kind);
    ASSERT_EQ(GAME_COMBAT_PHASE_START, out.events[0].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_PLAYER_DAMAGE, out.events[1].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_STATUS, out.events[2].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_MENU, out.events[3].arg0);
    PASS();
}

TEST game_bandit_intimidate_fail(void)
{
    struct GameState game;
    GameEventQueue out;
    int rolls[1];

    unit_game_fresh(&game, 11u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_event_queue_reset(&out);
    enemy_begin_encounter(&game, &out);
    rolls[0] = CFG_TEST_INTIMIDATE_FAIL;
    game_roll_inject_begin(&game, rolls, 1);
    ASSERT_EQ(1, run_cmd_out(&game, "3", &out));
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    ASSERT_EQ(5, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_INTIMIDATE, out.events[0].arg1);
    ASSERT_EQ(GAME_ENCOUNTER_OUTCOME_FAIL, out.events[0].arg2);
    ASSERT_EQ(GAME_EVENT_COMBAT, out.events[1].kind);
    ASSERT_EQ(GAME_COMBAT_PHASE_START, out.events[1].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_ENEMY_DAMAGE, out.events[2].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_STATUS, out.events[3].arg0);
    ASSERT_EQ(GAME_COMBAT_PHASE_MENU, out.events[4].arg0);
    PASS();
}

TEST game_bandit_handover_pick(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *bandit;
    int slot;

    unit_game_fresh(&game, 12u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_STICK);
    game_event_queue_reset(&out);
    enemy_begin_encounter(&game, &out);
    ASSERT_EQ(1, run_cmd_out(&game, "2", &out));
    slot = npc_find_by_dialogue(&game, DIALOGUE_ENEMY);
    bandit = slot >= 0 ? &game.npcs[slot] : 0;
    ASSERT(bandit != 0);
    /* handover gating reads NPC_FLAG_HANDOVER_PICK on the active enemy slot */
    ASSERT_EQ(NPC_FLAG_HANDOVER_PICK,
        bandit->flags & NPC_FLAG_HANDOVER_PICK);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_HANDOVER_PROMPT, out.events[0].arg1);
    PASS();
}

TEST game_wait_on_road_bandit_room_opens_encounter(void)
{
    struct GameState game;
    GameEventQueue out;
    int i;
    int saw_open;

    unit_game_fresh(&game, 12u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ROAD, 0);
    npc_deactivate_until(&game, GAME_DIALOGUE_ACTOR_TRAVELER, 999999UL);
    ASSERT_EQ(1, run_cmd_out(&game, "wait", &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_ENEMY, game.dialogue);
    saw_open = 0;
    for (i = 0; i < out.count; ++i) {
        if (out.events[i].kind == GAME_EVENT_ENCOUNTER &&
                out.events[i].arg0 == GAME_ENCOUNTER_BANDIT &&
                out.events[i].arg1 == GAME_ENCOUNTER_ACTION_OPEN) {
            saw_open = 1;
        }
    }
    ASSERT_EQ(1, saw_open);
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

TEST game_watchman_meal_thread_give_fed(void)
{
    struct GameState game;

    unit_game_fresh(&game, 230u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    game_inv_bag_add(&game, ITEM_BERRY);
    ASSERT_EQ(1, run_cmd(&game, "talk"));
    ASSERT_EQ(1, run_cmd(&game, "2"));
    ASSERT_EQ(WATCHMAN_SCENE_MEAL_OFFER, game.watchman_menu);
    ASSERT_EQ(1, run_cmd(&game, "give berry"));
    ASSERT_EQ(WATCHMAN_FLAG_FED, game.watchman_flags);
    ASSERT_EQ(1, gwhok_has(&game, WORLD_ADV_TOWER_MEAL));
    ASSERT_STR_EQ(TXT_STORY_TOWER_FED_DESC,
        game.world.rooms[WORLD_ROOM_TOWER].desc);
    ASSERT_EQ(0, game_inv_player_has_item(&game, ITEM_BERRY));
    ASSERT_EQ(WATCHMAN_SCENE_NEUTRAL, game.watchman_menu);
    PASS();
}

TEST game_fixture_baseline_clears_tower_advancement_desc(void)
{
    struct GameState game;

    unit_game_fresh(&game, 232u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    game_inv_bag_add(&game, ITEM_BERRY);
    ASSERT_EQ(1, run_cmd(&game, "talk"));
    ASSERT_EQ(1, run_cmd(&game, "2"));
    ASSERT_EQ(1, run_cmd(&game, "give berry"));
    ASSERT_STR_EQ(TXT_STORY_TOWER_FED_DESC,
        game.world.rooms[WORLD_ROOM_TOWER].desc);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    ASSERT_STR_EQ(g_room_descs[WORLD_ROOM_TOWER],
        game.world.rooms[WORLD_ROOM_TOWER].desc);
    PASS();
}

TEST game_bag_preserves_bandit_handover_dialogue(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *bandit;
    int slot;

    unit_game_fresh(&game, 231u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_STICK);
    game_event_queue_reset(&out);
    enemy_begin_encounter(&game, &out);
    ASSERT_EQ(1, run_cmd_out(&game, "2", &out));
    slot = npc_find_by_dialogue(&game, DIALOGUE_ENEMY);
    bandit = slot >= 0 ? &game.npcs[slot] : 0;
    ASSERT(bandit != 0);
    ASSERT_EQ(1, run_cmd_out(&game, "bag", &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_ENEMY, game.dialogue);
    ASSERT_EQ(NPC_FLAG_HANDOVER_PICK, bandit->flags & NPC_FLAG_HANDOVER_PICK);
    ASSERT_EQ(GAME_EVENT_BAG_VIEW, out.events[out.count - 1].kind);
    PASS();
}

TEST game_bag_preserves_watchman_meal_offer_dialogue(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 232u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    game_inv_bag_add(&game, ITEM_BERRY);
    ASSERT_EQ(1, run_cmd_out(&game, "talk", &out));
    ASSERT_EQ(1, run_cmd_out(&game, "2", &out));
    ASSERT_EQ(WATCHMAN_SCENE_MEAL_OFFER, game.watchman_menu);
    ASSERT_EQ(1, run_cmd_out(&game, "bag", &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_WATCHMAN, game.dialogue);
    ASSERT_EQ(WATCHMAN_SCENE_MEAL_OFFER, game.watchman_menu);
    ASSERT_EQ(GAME_EVENT_BAG_VIEW, out.events[out.count - 1].kind);
    PASS();
}

TEST game_bag_preserves_herbalist_give_dialogue(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 233u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_REQUESTED;
    game.marsh_root_spawned = 1;
    game_inv_bag_add(&game, ITEM_MARSH_ROOT);
    ASSERT_EQ(1, run_cmd_out(&game, "talk", &out));
    ASSERT_EQ(1, run_cmd_out(&game, "bag", &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_HERBALIST, game.dialogue);
    ASSERT_EQ(GAME_EVENT_BAG_VIEW, out.events[out.count - 1].kind);
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
    GameEventQueue out;

    unit_game_fresh(&game, 15u);
    ASSERT_EQ(1, testharn_apply(&game, "@fixture bandit_combat_turn1"));
    ASSERT_EQ(0, run_cmd(&game, "take stick"));
    ASSERT_EQ(0, run_cmd_out(&game, "look", &out));
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_BANDIT_WAITING_REPLY, out.events[0].arg0);
    ASSERT_EQ(GAME_EVENT_COMBAT, out.events[1].kind);
    ASSERT_EQ(GAME_COMBAT_PHASE_MENU, out.events[1].arg0);
    PASS();
}

TEST game_combat_use_salve_allowed(void)
{
    struct GameState game;
    GameEventQueue out;
    u32 tick_before_use;

    unit_game_fresh(&game, 115u);
    ASSERT_EQ(1, testharn_apply(&game, "@fixture bandit_combat_salve_ready"));
    tick_before_use = game.tick;
    ASSERT_EQ(1, run_cmd_out(&game, "use salve", &out));
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    ASSERT_EQ(tick_before_use + 1, game.tick);
    ASSERT(out.count >= 1);
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ITEM_ACTION_USE, out.events[0].arg0);
    PASS();
}

TEST game_inspect_none_and_inactive_kind(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 16u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, run_cmd_out(&game, "inspect", &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_OBSERVATION, out.events[0].kind);
    ASSERT_EQ(GAME_OBS_OUTCOME_NOTHING, out.events[0].arg0);
    game.env_room_clues[WORLD_ROOM_CAMP] |= (u8)(1u << (GAME_ENV_RUSTLE - 1));
    ASSERT_EQ(1, run_cmd_out(&game, "inspect water", &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_OBSERVATION, out.events[0].kind);
    ASSERT_EQ(GAME_OBS_OUTCOME_LEAD_SPENT, out.events[0].arg0);
    ASSERT_EQ(GAME_ENV_WATER, out.events[0].arg1);
    ASSERT_NEQ(0, game.env_room_clues[WORLD_ROOM_CAMP] &
        (u8)(1u << (GAME_ENV_RUSTLE - 1)));
    PASS();
}

TEST game_move_clears_departed_room_clues(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 18u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.env_room_clues[WORLD_ROOM_CAMP] =
        (u8)((1u << (GAME_ENV_RUSTLE - 1)) | (1u << (GAME_ENV_WATER - 1)));
    game_event_queue_reset(&out);
    ASSERT_EQ(1, run_cmd_out(&game, "north", &out));
    ASSERT_EQ(0, game.env_room_clues[WORLD_ROOM_CAMP]);
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
    GameEventQueue out;

    unit_game_fresh(&game, 19u);
    testharn_apply(&game, "@fixture bandit_handover_pick");
    ASSERT_EQ(1, run_cmd_out(&game, "give stick", &out));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_GIVE, out.events[0].arg1);
    ASSERT_EQ(GAME_ENCOUNTER_OUTCOME_OK, out.events[0].arg2);
    ASSERT_EQ(ITEM_STICK, out.events[0].arg3);
    ASSERT_EQ(GAME_EVENT_ROOM_LOOK, out.events[1].kind);
    PASS();
}

TEST game_give_to_herbalist_routes_room_npc_exchange(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 19u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_REQUESTED;
    game.marsh_root_spawned = 1;
    ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_MARSH_ROOT));
    ASSERT_EQ(1, run_cmd_out(&game, "give marsh-root", &out));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(HERBALIST_STORY_COMPLETE, game.herbalist_story);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(HERBALIST_SCENE_GIVE_REWARD_BAG, out.events[0].arg3);
    PASS();
}

TEST game_give_in_enemy_dialogue_beats_room_npc_exchange(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 191u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_REQUESTED;
    ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_STICK));
    ASSERT(npc_spawn(&game, GAME_DIALOGUE_ACTOR_BANDIT_AMBUSH, DIALOGUE_ENEMY,
        GAME_ENCOUNTER_BANDIT, WORLD_ROOM_ORCHARD,
        NPC_FLAG_ACTIVE | NPC_FLAG_HANDOVER_PICK) >= 0);
    game_set_mode_dialogue(&game, DIALOGUE_ENEMY);
    ASSERT_EQ(1, run_cmd_out(&game, "give stick", &out));
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_GIVE, out.events[0].arg1);
    ASSERT_EQ(GAME_ENCOUNTER_OUTCOME_OK, out.events[0].arg2);
    ASSERT_EQ(ITEM_STICK, out.events[0].arg3);
    ASSERT_EQ(GAME_EVENT_ROOM_LOOK, out.events[1].kind);
    PASS();
}

TEST game_give_closes_traveler_dialogue_before_room_npc_exchange(void)
{
    struct GameState game;
    GameEventQueue out;
    int slot;

    unit_game_fresh(&game, 192u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_REQUESTED;
    game.marsh_root_spawned = 1;
    ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_MARSH_ROOT));
    slot = npc_find_by_actor(&game, GAME_DIALOGUE_ACTOR_TRAVELER);
    ASSERT(slot >= 0);
    game.npcs[slot].room_id = WORLD_ROOM_ORCHARD;
    game.npcs[slot].dialogue = DIALOGUE_TRAVELER;
    game.npcs[slot].flags |= NPC_FLAG_ACTIVE;
    game_set_mode_dialogue(&game, DIALOGUE_TRAVELER);
    ASSERT_EQ(1, run_cmd_out(&game, "give marsh-root", &out));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(HERBALIST_STORY_COMPLETE, game.herbalist_story);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_DIALOGUE_CLOSED, out.events[0].arg0);
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[1].kind);
    ASSERT_EQ(HERBALIST_SCENE_GIVE_REWARD_BAG, out.events[1].arg3);
    PASS();
}

TEST game_give_without_npc_target_is_guarded(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 19u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_STICK));
    ASSERT_EQ(1, run_cmd_out(&game, "give stick", &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_GIVE_NO_TARGET, out.events[0].arg0);
    PASS();
}

TEST game_traveler_reply_fixture(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 20u);
    testharn_apply(&game, "@fixture traveler_dialogue");
    ASSERT_EQ(1, run_cmd_out(&game, "reply 2", &out));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(0, traveler_npc(&game)->flags & NPC_FLAG_ACTIVE);
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_TRAVELER, out.events[0].arg0);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_REPLY, out.events[0].arg1);
    ASSERT_EQ(2, out.events[0].arg2);
    ASSERT_EQ(GAME_EVENT_ROOM_LOOK, out.events[1].kind);
    PASS();
}

TEST game_wait_and_help(void)
{
    struct GameState game;

    unit_game_fresh(&game, 21u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.test_quiet_ticks = 1;
    disable_traveler(&game);
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
    disable_traveler(&game);
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
    disable_traveler(&game);
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
    disable_traveler(&game);
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
    disable_traveler(&game);
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
    disable_traveler(&game);
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
    disable_traveler(&game);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, game_process_input(&game, line, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_MAP, out.events[0].kind);
    PASS();
}

TEST game_version_emits_version_event_without_tick(void)
{
    struct GameState game;
    GameEventQueue out;
    char line[] = "version";

    unit_game_fresh(&game, 40u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.test_quiet_ticks = 1;
    disable_traveler(&game);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, game_process_input(&game, line, &out));
    ASSERT_EQ(0, game.tick);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_VERSION, out.events[0].kind);
    ASSERT(out.events[0].text != 0);
    ASSERT(strstr(out.events[0].text, "dosmud ") == out.events[0].text);
    PASS();
}

TEST game_version_allowed_during_bandit_dialogue(void)
{
    struct GameState game;
    GameEventQueue out;
    char line[] = "version";

    unit_game_fresh(&game, 45u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_event_queue_reset(&out);
    enemy_begin_encounter(&game, &out);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, game_process_input(&game, line, &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(0, game.tick);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_VERSION, out.events[0].kind);
    PASS();
}

TEST game_version_allowed_during_combat(void)
{
    struct GameState game;
    GameEventQueue out;
    char line[] = "version";
    u32 tick_before_version;

    unit_game_fresh(&game, 46u);
    ASSERT_EQ(1, testharn_apply(&game, "@fixture bandit_combat_turn1"));
    tick_before_version = game.tick;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, game_process_input(&game, line, &out));
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    ASSERT_EQ(tick_before_version, game.tick);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_VERSION, out.events[0].kind);
    PASS();
}

TEST game_version_allowed_during_loot_menu(void)
{
    struct GameState game;
    GameEventQueue out;
    char line[] = "version";

    unit_game_fresh(&game, 47u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.corpse_present[WORLD_ROOM_CAMP] = 1;
    game.corpse_item[WORLD_ROOM_CAMP][0] = ITEM_HERB;
    ASSERT_EQ(1, run_cmd_out(&game, "loot", &out));
    ASSERT_EQ(1, game.tick);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_LOOT, game.dialogue);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, game_process_input(&game, line, &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_LOOT, game.dialogue);
    ASSERT_EQ(1, game.tick);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_VERSION, out.events[0].kind);
    PASS();
}

TEST game_traveler_dialogue_blocks_inspect_and_replays_scene(void)
{
    struct GameState game;
    GameEventQueue out;
    u32 tick_before_inspect;

    unit_game_fresh(&game, 48u);
    ASSERT_EQ(1, testharn_apply(&game, "@fixture traveler_dialogue"));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_TRAVELER, game.dialogue);

    tick_before_inspect = game.tick;
    ASSERT_EQ(0, run_cmd_out(&game, "inspect", &out));
    ASSERT_EQ(tick_before_inspect, game.tick);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_TRAVELER, game.dialogue);
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_ROAMING_ENCOUNTER_WAITING,
        out.events[0].arg0);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[1].kind);
    ASSERT_EQ(GAME_ENCOUNTER_TRAVELER, out.events[1].arg0);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_OPEN, out.events[1].arg1);
    ASSERT_EQ(WORLD_ROOM_ROAD, game.player.room_id);
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
    disable_traveler(&game);
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
    disable_traveler(&game);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, game_process_input(&game, line, &out));
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_MOVE, out.events[0].kind);
    ASSERT_STR_EQ("north", out.events[0].text);
    ASSERT_EQ(GAME_EVENT_ROOM_LOOK, out.events[1].kind);
    PASS();
}

TEST game_move_into_bandit_defers_room_look_until_after_encounter(void)
{
    struct GameState game;
    GameEventQueue out;
    int slot;
    int i;
    int saw_look;

    unit_game_fresh(&game, 135u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    disable_traveler(&game);
    slot = npc_find_by_actor(&game, GAME_DIALOGUE_ACTOR_BANDIT);
    ASSERT(slot >= 0);
    game.npcs[slot].flags |= NPC_FLAG_ACTIVE | NPC_FLAG_ROAMING;
    game.npcs[slot].flags &= ~NPC_FLAG_NEEDS_SEPARATION;
    game.npcs[slot].room_id = WORLD_ROOM_ROAD;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, run_cmd_out(&game, "move north", &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_ENEMY, game.dialogue);
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_MOVE, out.events[0].kind);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[1].kind);
    ASSERT_EQ(GAME_ENCOUNTER_BANDIT, out.events[1].arg0);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_OPEN, out.events[1].arg1);
    saw_look = 0;
    for (i = 0; i < out.count; ++i) {
        if (out.events[i].kind == GAME_EVENT_ROOM_LOOK) {
            saw_look = 1;
        }
    }
    ASSERT_EQ(0, saw_look);
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
    game_event_queue_reset(&out);
    enemy_begin_encounter(&game, &out);
    game_event_queue_reset(&out);
    ASSERT_EQ(0, game_process_input(&game, line, &out));
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_BANDIT_WAITING_REPLY, out.events[0].arg0);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[1].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_OPEN, out.events[1].arg1);
    PASS();
}

TEST game_bandit_map_replays_modal_prompt(void)
{
    struct GameState game;
    GameEventQueue out;
    u32 tick_before_map;

    unit_game_fresh(&game, 140u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_event_queue_reset(&out);
    enemy_begin_encounter(&game, &out);
    tick_before_map = game.tick;
    ASSERT_EQ(0, run_cmd_out(&game, "map", &out));
    ASSERT_EQ(tick_before_map, game.tick);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_BANDIT_WAITING_REPLY, out.events[0].arg0);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[1].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_OPEN, out.events[1].arg1);
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

TEST game_post_combat_reply_guard_keeps_loot_available(void)
{
    struct GameState game;
    GameEventQueue out;
    u32 tick_before_reply;

    unit_game_fresh(&game, 42u);
    ASSERT_EQ(1, testharn_apply(&game, "@fixture bandit_victory_herb"));

    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    ASSERT_EQ(1, run_cmd(&game, "1"));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(ITEM_HERB, game.corpse_item[WORLD_ROOM_CAMP][0]);
    ASSERT_EQ(1, game.corpse_present[WORLD_ROOM_CAMP]);

    tick_before_reply = game.tick;
    ASSERT_EQ(1, run_cmd_out(&game, "1", &out));
    ASSERT_EQ(tick_before_reply, game.tick);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_NOBODY_WAITING_REPLY, out.events[0].arg0);
    ASSERT_EQ(ITEM_HERB, game.corpse_item[WORLD_ROOM_CAMP][0]);
    ASSERT_EQ(1, game.corpse_present[WORLD_ROOM_CAMP]);

    ASSERT_EQ(1, run_cmd_out(&game, "loot", &out));
    ASSERT_EQ(tick_before_reply + 1, game.tick);
    ASSERT(out.count >= 1);
    ASSERT_EQ(GAME_EVENT_CORPSE_VIEW, out.events[0].kind);
    ASSERT_EQ(1, out.events[0].arg0);
    ASSERT_EQ(1, run_cmd_out(&game, "1", &out));
    ASSERT_EQ(tick_before_reply + 1, game.tick);
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ITEM_ACTION_LOOT, out.events[0].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(ITEM_HERB, out.events[0].arg2);
    ASSERT_EQ(ITEM_NONE, game.corpse_item[WORLD_ROOM_CAMP][0]);
    ASSERT_EQ(0, game.corpse_present[WORLD_ROOM_CAMP]);
    PASS();
}

TEST game_loot_leave_keeps_corpse_without_advancing_time(void)
{
    struct GameState game;
    GameEventQueue out;
    u32 tick_before_leave;

    unit_game_fresh(&game, 43u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.corpse_present[WORLD_ROOM_CAMP] = 1;
    game.corpse_item[WORLD_ROOM_CAMP][0] = ITEM_HERB;
    game.corpse_item[WORLD_ROOM_CAMP][1] = ITEM_NONE;
    game.corpse_item[WORLD_ROOM_CAMP][2] = ITEM_NONE;

    ASSERT_EQ(1, run_cmd_out(&game, "loot", &out));
    ASSERT_EQ(1, game.tick);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_LOOT, game.dialogue);
    ASSERT_EQ(GAME_EVENT_CORPSE_VIEW, out.events[0].kind);

    tick_before_leave = game.tick;
    ASSERT_EQ(1, run_cmd_out(&game, "2", &out));
    ASSERT_EQ(tick_before_leave, game.tick);
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ITEM_OUTCOME_LEFT_BEHIND, out.events[0].arg1);
    ASSERT_EQ(1, game.corpse_present[WORLD_ROOM_CAMP]);
    ASSERT_EQ(ITEM_HERB, game.corpse_item[WORLD_ROOM_CAMP][0]);
    PASS();
}

TEST game_loot_reply_four_leaves_three_item_corpse(void)
{
    struct GameState game;
    GameEventQueue out;
    u32 tick_before_leave;

    unit_game_fresh(&game, 44u);
    ASSERT_EQ(1, testharn_apply(&game, "@fixture bandit_victory_multi"));

    ASSERT_EQ(1, run_cmd(&game, "1"));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(1, run_cmd_out(&game, "loot", &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(GAME_EVENT_CORPSE_VIEW, out.events[0].kind);
    ASSERT_EQ(3, out.events[0].arg0);
    ASSERT_EQ(4, out.events[0].arg1);

    tick_before_leave = game.tick;
    ASSERT_EQ(1, run_cmd_out(&game, "4", &out));
    ASSERT_EQ(tick_before_leave, game.tick);
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ITEM_OUTCOME_LEFT_BEHIND, out.events[0].arg1);
    ASSERT_EQ(1, game.corpse_present[WORLD_ROOM_CAMP]);
    ASSERT_EQ(ITEM_STICK, game.corpse_item[WORLD_ROOM_CAMP][0]);
    ASSERT_EQ(ITEM_BERRY, game.corpse_item[WORLD_ROOM_CAMP][1]);
    ASSERT_EQ(ITEM_HERB, game.corpse_item[WORLD_ROOM_CAMP][2]);
    PASS();
}

TEST game_drop_allowed_while_loot_menu_open_after_bag_full(void)
{
    struct GameState game;
    GameEventQueue out;
    u32 tick_before_drop;

    unit_game_fresh(&game, 45u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.test_quiet_ticks = 1;
    game.corpse_present[WORLD_ROOM_CAMP] = 1;
    game.corpse_item[WORLD_ROOM_CAMP][0] = ITEM_BERRY;
    game.corpse_item[WORLD_ROOM_CAMP][1] = ITEM_NONE;
    game.corpse_item[WORLD_ROOM_CAMP][2] = ITEM_NONE;
    game.bag[0] = ITEM_STICK;
    game.bag_count = 1;
    game.bag_capacity = 1;

    ASSERT_EQ(1, run_cmd_out(&game, "loot", &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(1, run_cmd_out(&game, "1", &out));
    ASSERT_EQ(GAME_ITEM_OUTCOME_BAG_FULL_DROP, out.events[0].arg1);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);

    tick_before_drop = game.tick;
    ASSERT_EQ(1, run_cmd_out(&game, "drop stick", &out));
    ASSERT_EQ(tick_before_drop + 1U, game.tick);
    ASSERT(out.count >= 2);
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ITEM_ACTION_LOOT, out.events[0].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_LEFT_BEHIND, out.events[0].arg1);
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[1].kind);
    ASSERT_EQ(GAME_ITEM_ACTION_DROP, out.events[1].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[1].arg1);
    ASSERT_EQ(0, game.bag_count);
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST game_loot_menu_closes_before_bag_view(void)
{
    struct GameState game;
    GameEventQueue out;
    u32 tick_before_bag;

    unit_game_fresh(&game, 49u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.corpse_present[WORLD_ROOM_CAMP] = 1;
    game.corpse_item[WORLD_ROOM_CAMP][0] = ITEM_HERB;

    ASSERT_EQ(1, run_cmd_out(&game, "loot", &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_LOOT, game.dialogue);

    tick_before_bag = game.tick;
    ASSERT_EQ(1, run_cmd_out(&game, "bag", &out));
    ASSERT_EQ(tick_before_bag, game.tick);
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(DIALOGUE_NONE, game.dialogue);
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_ITEM_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ITEM_ACTION_LOOT, out.events[0].arg0);
    ASSERT_EQ(GAME_ITEM_OUTCOME_LEFT_BEHIND, out.events[0].arg1);
    ASSERT_EQ(GAME_EVENT_BAG_VIEW, out.events[1].kind);
    ASSERT_EQ(1, game.corpse_present[WORLD_ROOM_CAMP]);
    ASSERT_EQ(ITEM_HERB, game.corpse_item[WORLD_ROOM_CAMP][0]);
    PASS();
}

TEST game_loot_all_stops_at_bag_full_without_advancing_time(void)
{
    struct GameState game;
    GameEventQueue out;
    u32 tick_before_loot_all;

    unit_game_fresh(&game, 46u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.corpse_present[WORLD_ROOM_CAMP] = 1;
    game.corpse_item[WORLD_ROOM_CAMP][0] = ITEM_BERRY;
    game.corpse_item[WORLD_ROOM_CAMP][1] = ITEM_STICK;
    game.corpse_item[WORLD_ROOM_CAMP][2] = ITEM_HERB;
    game.bag_capacity = 2;

    ASSERT_EQ(1, run_cmd_out(&game, "loot", &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    tick_before_loot_all = game.tick;
    ASSERT_EQ(1, run_cmd_out(&game, "loot all", &out));
    ASSERT_EQ(tick_before_loot_all, game.tick);
    ASSERT_EQ(4, out.count);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[0].arg1);
    ASSERT_EQ(GAME_ITEM_OUTCOME_OK, out.events[1].arg1);
    ASSERT_EQ(GAME_ITEM_OUTCOME_BAG_FULL_DROP, out.events[2].arg1);
    ASSERT_EQ(GAME_EVENT_CORPSE_VIEW, out.events[3].kind);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_LOOT, game.dialogue);
    ASSERT_EQ(ITEM_HERB, game.corpse_item[WORLD_ROOM_CAMP][0]);
    PASS();
}

TEST game_herbalist_request_then_take_root(void)
{
    struct GameState game;

    unit_game_fresh(&game, 220u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    ASSERT_EQ(1, run_cmd(&game, "talk"));
    ASSERT_EQ(1, run_cmd(&game, "1"));
    ASSERT_EQ(HERBALIST_STORY_REQUESTED, game.herbalist_story);
    ASSERT(room_has_item(&game, WORLD_ROOM_MARSH, ITEM_MARSH_ROOT));

    game.player.room_id = WORLD_ROOM_MARSH;
    game.room_explored[WORLD_ROOM_MARSH] = 1;
    ASSERT_EQ(1, run_cmd(&game, "take marsh-root"));
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_MARSH_ROOT));
    ASSERT(!room_has_item(&game, WORLD_ROOM_MARSH, ITEM_MARSH_ROOT));
    PASS();
}

TEST game_herbalist_turn_in_updates_orchard_desc(void)
{
    struct GameState game;

    unit_game_fresh(&game, 221u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_REQUESTED;
    game.marsh_root_spawned = 1;
    game_inv_bag_add(&game, ITEM_MARSH_ROOT);

    ASSERT_EQ(1, run_cmd(&game, "talk"));
    ASSERT_EQ(1, run_cmd(&game, "1"));
    ASSERT_EQ(HERBALIST_STORY_COMPLETE, game.herbalist_story);
    ASSERT_EQ(1, gwhok_has(&game, WORLD_ADV_ORCHARD_RESTORED));
    ASSERT_STR_EQ(TXT_STORY_ORCHARD_DONE_DESC,
        game.world.rooms[WORLD_ROOM_ORCHARD].desc);
    PASS();
}

TEST game_herbalist_retry_seed_when_marsh_slot_frees(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 222u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.room_item[WORLD_ROOM_MARSH][0] = ITEM_REED;
    game.room_item[WORLD_ROOM_MARSH][1] = ITEM_STONE;
    game.room_item[WORLD_ROOM_MARSH][2] = ITEM_BERRY;
    game.room_item[WORLD_ROOM_MARSH][3] = ITEM_HERB;
    ASSERT_EQ(1, run_cmd(&game, "talk"));
    ASSERT_EQ(1, run_cmd(&game, "1"));
    ASSERT_EQ(HERBALIST_STORY_REQUESTED, game.herbalist_story);
    ASSERT_EQ(0, game.marsh_root_spawned);
    ASSERT(!room_has_item(&game, WORLD_ROOM_MARSH, ITEM_MARSH_ROOT));

    game.room_item[WORLD_ROOM_MARSH][3] = ITEM_NONE;
    game_event_queue_reset(&out);
    game_background_step(&game, &out);
    ASSERT_EQ(1, game.marsh_root_spawned);
    ASSERT_EQ(ITEM_MARSH_ROOT, game.room_item[WORLD_ROOM_MARSH][3]);
    PASS();
}

TEST game_herbalist_drop_root_outside_marsh_does_not_duplicate(void)
{
    struct GameState game;

    unit_game_fresh(&game, 223u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    ASSERT_EQ(1, run_cmd(&game, "talk"));
    ASSERT_EQ(1, run_cmd(&game, "1"));
    ASSERT_EQ(HERBALIST_STORY_REQUESTED, game.herbalist_story);
    ASSERT(room_has_item(&game, WORLD_ROOM_MARSH, ITEM_MARSH_ROOT));

    game.player.room_id = WORLD_ROOM_MARSH;
    game.room_explored[WORLD_ROOM_MARSH] = 1;
    ASSERT_EQ(1, run_cmd(&game, "take marsh-root"));
    game.player.room_id = WORLD_ROOM_ROAD;
    game.room_explored[WORLD_ROOM_ROAD] = 1;
    ASSERT_EQ(1, run_cmd(&game, "drop root"));
    ASSERT(room_has_item(&game, WORLD_ROOM_ROAD, ITEM_MARSH_ROOT));
    ASSERT(!room_has_item(&game, WORLD_ROOM_MARSH, ITEM_MARSH_ROOT));
    PASS();
}

TEST game_reset_fixture_baseline_initializes_weather(void)
{
    struct GameState game;

    unit_game_fresh(&game, 51u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(GAME_WEATHER_NONE, game.weather_kind);
    ASSERT_EQ((u32)CFG_WEATHER_INITIAL_DELAY_TICKS, game.weather_expires_tick);
    PASS();
}

TEST game_reset_fixture_baseline_initializes_daynight(void)
{
    struct GameState game;

    unit_game_fresh(&game, 130u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(GAME_DAY, game.day_phase);
    ASSERT_EQ((u32)CFG_DAYNIGHT_INITIAL_DELAY_TICKS, game.day_expires_tick);
    ASSERT_EQ(0, game.night_lost);
    PASS();
}

/* seed 1234 + tick 0 hash roll triggers lost without torch (#130). */
TEST game_night_move_without_torch_sets_lost(void)
{
    struct GameState game;
    GameEventQueue out;
    int i;
    int found;

    unit_game_fresh(&game, 1234u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.day_phase = GAME_NIGHT;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, run_cmd_out(&game, "north", &out));
    found = 0;
    for (i = 0; i < out.count; ++i) {
        if (out.events[i].kind == GAME_EVENT_ENVIRONMENT &&
                out.events[i].arg0 == GAME_ENV_EVENT_NIGHT_LOST) {
            found = 1;
        }
    }
    ASSERT_EQ(1, found);
    ASSERT_EQ(1, game.night_lost);
    PASS();
}

TEST game_fog_blocks_encounter_still_roams(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *traveler;
    struct NpcState *bandit;
    int before;
    int bandit_slot;

    unit_game_fresh(&game, 1234u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    traveler = traveler_npc(&game);
    bandit_slot = npc_find_by_actor(&game, GAME_DIALOGUE_ACTOR_BANDIT);
    ASSERT(traveler != 0);
    ASSERT(bandit_slot >= 0);
    bandit = &game.npcs[bandit_slot];
    traveler->room_id = WORLD_ROOM_CAMP;
    traveler->flags |= NPC_FLAG_ACTIVE | NPC_FLAG_ROAMING;
    bandit->flags &= ~NPC_FLAG_ACTIVE;
    game.weather_kind = GAME_WEATHER_FOG;
    game.weather_expires_tick = 100;
    game.tick = 3;
    plat_seed_rng(42u);
    before = traveler->room_id;
    game_event_queue_reset(&out);
    game_background_step(&game, &out);
    ASSERT_EQ(4, (int)game.tick);
    ASSERT_EQ(1, gatmos_weather_blocks_roaming_encounter(&game));
    ASSERT_NEQ(before, traveler->room_id);
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
    RUN_TEST(game_reset_fixture_baseline_initializes_weather);
    RUN_TEST(game_reset_fixture_baseline_initializes_daynight);
    RUN_TEST(game_night_move_without_torch_sets_lost);
    RUN_TEST(game_fog_blocks_encounter_still_roams);
    RUN_TEST(game_bandit_intimidate_success);
    RUN_TEST(game_inspect_with_focus);
    RUN_TEST(game_env_inspect_reply);
    RUN_TEST(game_env_menu_dismiss_on_move);
    RUN_TEST(game_env_reply_room_mismatch_fail_closed);
    RUN_TEST(game_talk_frog);
    RUN_TEST(game_bandit_fight_reply);
    RUN_TEST(game_bandit_intimidate_fail);
    RUN_TEST(game_bandit_handover_pick);
    RUN_TEST(game_wait_on_road_bandit_room_opens_encounter);
    RUN_TEST(game_talk_npcs_and_nobody);
    RUN_TEST(game_watchman_meal_thread_give_fed);
    RUN_TEST(game_fixture_baseline_clears_tower_advancement_desc);
    RUN_TEST(game_bag_preserves_bandit_handover_dialogue);
    RUN_TEST(game_bag_preserves_watchman_meal_offer_dialogue);
    RUN_TEST(game_bag_preserves_herbalist_give_dialogue);
    RUN_TEST(game_frog_reply_branch);
    RUN_TEST(game_combat_blocks_inventory_cmds);
    RUN_TEST(game_combat_use_salve_allowed);
    RUN_TEST(game_inspect_none_and_inactive_kind);
    RUN_TEST(game_move_clears_departed_room_clues);
    RUN_TEST(game_unknown_command);
    RUN_TEST(game_give_after_handover_fixture);
    RUN_TEST(game_give_in_enemy_dialogue_beats_room_npc_exchange);
    RUN_TEST(game_give_closes_traveler_dialogue_before_room_npc_exchange);
    RUN_TEST(game_give_to_herbalist_routes_room_npc_exchange);
    RUN_TEST(game_give_without_npc_target_is_guarded);
    RUN_TEST(game_traveler_reply_fixture);
    RUN_TEST(game_wait_and_help);
    RUN_TEST(game_session_help_no_tick);
    RUN_TEST(game_observe_look_no_tick);
    RUN_TEST(game_pass_time_wait_ticks);
    RUN_TEST(game_wait_emits_output_record);
    RUN_TEST(game_help_emits_help_event);
    RUN_TEST(game_map_emits_map_event);
    RUN_TEST(game_version_emits_version_event_without_tick);
    RUN_TEST(game_version_allowed_during_bandit_dialogue);
    RUN_TEST(game_version_allowed_during_combat);
    RUN_TEST(game_version_allowed_during_loot_menu);
    RUN_TEST(game_traveler_dialogue_blocks_inspect_and_replays_scene);
    RUN_TEST(game_unknown_command_emits_event);
    RUN_TEST(game_cannot_move_emits_event);
    RUN_TEST(game_move_emits_move_then_look);
    RUN_TEST(game_move_into_bandit_defers_room_look_until_after_encounter);
    RUN_TEST(game_roll_spread_zero);
    RUN_TEST(game_quit_ends_run);
    RUN_TEST(game_bandit_waiting_reply_guard_event);
    RUN_TEST(game_bandit_map_replays_modal_prompt);
    RUN_TEST(game_nobody_waiting_reply_guard_event);
    RUN_TEST(game_post_combat_reply_guard_keeps_loot_available);
    RUN_TEST(game_loot_leave_keeps_corpse_without_advancing_time);
    RUN_TEST(game_loot_reply_four_leaves_three_item_corpse);
    RUN_TEST(game_drop_allowed_while_loot_menu_open_after_bag_full);
    RUN_TEST(game_loot_menu_closes_before_bag_view);
    RUN_TEST(game_loot_all_stops_at_bag_full_without_advancing_time);
    RUN_TEST(game_herbalist_request_then_take_root);
    RUN_TEST(game_herbalist_turn_in_updates_orchard_desc);
    RUN_TEST(game_herbalist_retry_seed_when_marsh_slot_frees);
    RUN_TEST(game_herbalist_drop_root_outside_marsh_does_not_duplicate);
}
