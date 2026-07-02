#include "greatest.h"
#include "config.h"
#include "game.h"
#include "gout.h"
#include "gwhok.h"
#include "invent.h"
#include "items.h"
#include "npc.h"
#include "platform.h"
#include "txtres.h"
#include "world.h"
#include "unit_util.h"

/* Match bandit row in NPC_PROFILES (npc.c) for unit assertions. */
#define PROFILE_BANDIT_LEVEL_MIN 1
#define PROFILE_BANDIT_LEVEL_MAX 3
#define PROFILE_BANDIT_ROAM_START 8U
#define PROFILE_BANDIT_RETURN_BASE 6U
#define PROFILE_BANDIT_RETURN_SPREAD 10U
#define PROFILE_BANDIT_BRIDGE_SPAWN WORLD_ROOM_BRIDGE
#define PROFILE_BANDIT_CANYON_SPAWN WORLD_ROOM_CANYON

/*
 * Direct npc.c API tests: room/actor lookup, roaming movement/encounter, and
 * open-room dialogue without going through game_process_input.
 */

TEST npc_room_actor_lookup(void)
{
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_NONE, npc_room_actor(WORLD_ROOM_CAMP));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_FROG, npc_room_actor(WORLD_ROOM_POND));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_WATCHMAN, npc_room_actor(WORLD_ROOM_TOWER));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_HERBALIST, npc_room_actor(WORLD_ROOM_ORCHARD));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_ARCHIVIST,
        npc_room_actor(WORLD_ROOM_CATACOMBS));
    PASS();
}

TEST npc_dialogue_actor_lookup(void)
{
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_NONE, npc_dialogue_actor(DIALOGUE_NONE));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_FROG, npc_dialogue_actor(DIALOGUE_NPC_FROG));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_WATCHMAN,
        npc_dialogue_actor(DIALOGUE_NPC_WATCHMAN));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_HERBALIST,
        npc_dialogue_actor(DIALOGUE_NPC_HERBALIST));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_ARCHIVIST,
        npc_dialogue_actor(DIALOGUE_NPC_ARCHIVIST));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_NONE,
        npc_dialogue_actor(DIALOGUE_TRAVELER));
    PASS();
}

TEST npc_choice_validation(void)
{
    ASSERT_EQ(0, npc_choice_is_valid(0));
    ASSERT_EQ(1, npc_choice_is_valid(1));
    ASSERT_EQ(1, npc_choice_is_valid(3));
    ASSERT_EQ(0, npc_choice_is_valid(4));
    PASS();
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

TEST npc_open_room_dialogue_frog(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 30u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_open_room_dialogue(&game, &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_FROG, game.dialogue);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_FROG, out.events[0].arg0);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_TALK, out.events[0].arg1);
    PASS();
}

TEST npc_open_room_dialogue_watchman(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 31u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_open_room_dialogue(&game, &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_WATCHMAN, game.dialogue);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_WATCHMAN, out.events[0].arg0);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_TALK, out.events[0].arg1);
    ASSERT_EQ(WATCHMAN_SCENE_NEUTRAL, out.events[0].arg3);
    PASS();
}

TEST npc_open_room_dialogue_watchman_always_neutral_entry(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 32u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    game.watchman_flags = WATCHMAN_FLAG_WARNED;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_open_room_dialogue(&game, &out));
    ASSERT_EQ(WATCHMAN_SCENE_NEUTRAL_WARNED, out.events[0].arg3);
    ASSERT_EQ(WATCHMAN_SCENE_NEUTRAL, game.watchman_menu);
    PASS();
}

TEST npc_room_cmd_reply_watchman_meal_give_fed(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 321u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    game_inv_bag_add(&game, ITEM_FISH);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_open_room_dialogue(&game, &out));
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_room_cmd_reply(&game, 2, &out));
    ASSERT_EQ(WATCHMAN_SCENE_MEAL_OFFER, game.watchman_menu);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_cmd_give(&game, ITEM_FISH, &out));
    ASSERT_EQ(WATCHMAN_FLAG_FED, game.watchman_flags);
    ASSERT_EQ(1, gwhok_has(&game, WORLD_ADV_TOWER_MEAL));
    ASSERT_STR_EQ(TXT_STORY_TOWER_FED_DESC,
        game.world.rooms[WORLD_ROOM_TOWER].desc);
    ASSERT_EQ(0, game_inv_player_has_item(&game, ITEM_FISH));
    ASSERT_EQ(0, game_inv_player_has_item(&game, ITEM_HERB));
    ASSERT_EQ(WATCHMAN_SCENE_FOOD_THANKS, out.events[0].arg3);
    ASSERT_EQ(WATCHMAN_SCENE_NEUTRAL, game.watchman_menu);
    ASSERT_EQ(WATCHMAN_SCENE_NEUTRAL_FED, out.events[1].arg3);
    PASS();
}

TEST npc_room_cmd_reply_watchman_apologize_sets_promised(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 323u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_open_room_dialogue(&game, &out));
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_room_cmd_reply(&game, 2, &out));
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_room_cmd_reply(&game, 1, &out));
    ASSERT_EQ(WATCHMAN_FLAG_PROMISED, game.watchman_flags);
    ASSERT_EQ(WATCHMAN_SCENE_APOLOGY, out.events[0].arg3);
    PASS();
}

TEST npc_room_cmd_reply_watchman_meal_no_food(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 322u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_open_room_dialogue(&game, &out));
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_room_cmd_reply(&game, 2, &out));
    ASSERT_EQ(WATCHMAN_SCENE_MEAL_OFFER_EMPTY, game.watchman_menu);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_room_cmd_reply(&game, 1, &out));
    ASSERT_EQ(WATCHMAN_FLAG_PROMISED, game.watchman_flags);
    ASSERT_EQ(WATCHMAN_SCENE_APOLOGY, out.events[0].arg3);
    ASSERT_EQ(WATCHMAN_SCENE_NEUTRAL, game.watchman_menu);
    PASS();
}

TEST npc_open_room_dialogue_herbalist_requested_scene(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 311u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_REQUESTED;
    game.marsh_root_spawned = 1;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_open_room_dialogue(&game, &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_HERBALIST, game.dialogue);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_HERBALIST, out.events[0].arg0);
    ASSERT_EQ(HERBALIST_SCENE_REQUESTED, out.events[0].arg3);
    ASSERT_EQ(HERBALIST_SCENE_REQUESTED, game.herbalist_menu);
    PASS();
}

TEST npc_room_cmd_reply_herbalist_requested_root_opens_quest_submenu(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 317u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_REQUESTED;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_open_room_dialogue(&game, &out));
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_room_cmd_reply(&game, 1, &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(HERBALIST_SCENE_REQUESTED_OPTIONS, game.herbalist_menu);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(HERBALIST_SCENE_REQUESTED_OPTIONS, out.events[0].arg3);
    PASS();
}

TEST npc_room_cmd_reply_herbalist_requested_root_gossip_leaves_dialogue(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 318u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_REQUESTED;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_open_room_dialogue(&game, &out));
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_room_cmd_reply(&game, 2, &out));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(DIALOGUE_NONE, game.dialogue);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(HERBALIST_SCENE_NOT_STARTED, out.events[0].arg3);
    PASS();
}

TEST npc_open_room_dialogue_herbalist_reseeds_missing_root(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 313u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_REQUESTED;
    game.marsh_root_spawned = 1;
    game.room_item[WORLD_ROOM_MARSH][0] = ITEM_REED;
    game.room_item[WORLD_ROOM_MARSH][1] = ITEM_NONE;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_open_room_dialogue(&game, &out));
    ASSERT_EQ(1, game.marsh_root_spawned);
    ASSERT_EQ(ITEM_MARSH_ROOT, game.room_item[WORLD_ROOM_MARSH][1]);
    ASSERT_EQ(HERBALIST_SCENE_REQUESTED, out.events[0].arg3);
    PASS();
}

TEST npc_open_room_dialogue_herbalist_ready_scene(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 314u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_REQUESTED;
    game_inv_bag_add(&game, ITEM_MARSH_ROOT);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_open_room_dialogue(&game, &out));
    ASSERT_EQ(HERBALIST_SCENE_READY, out.events[0].arg3);
    PASS();
}

TEST npc_room_cmd_reply_herbalist_turn_in_updates_story(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 312u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_REQUESTED;
    game.marsh_root_spawned = 1;
    game_inv_bag_add(&game, ITEM_MARSH_ROOT);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_open_room_dialogue(&game, &out));
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_room_cmd_reply(&game, 1, &out));
    ASSERT_EQ(HERBALIST_STORY_COMPLETE, game.herbalist_story);
    ASSERT_EQ(1, gwhok_has(&game, WORLD_ADV_ORCHARD_RESTORED));
    ASSERT_STR_EQ(TXT_STORY_ORCHARD_DONE_DESC,
        game.world.rooms[WORLD_ROOM_ORCHARD].desc);
    ASSERT_EQ(-1, game_inv_bag_find_index(&game, ITEM_MARSH_ROOT));
    ASSERT_EQ(HERBALIST_SCENE_GIVE_REWARD_BAG, out.events[0].arg3);
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_SALVE));
    PASS();
}

TEST npc_cmd_give_herbalist_rejects_before_request(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 316u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_NONE;
    game_inv_bag_add(&game, ITEM_STICK);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_cmd_give(&game, ITEM_STICK, &out));
    ASSERT_EQ(HERBALIST_STORY_NONE, game.herbalist_story);
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_STICK));
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_GIVE_REJECTED, out.events[0].arg0);
    PASS();
}

TEST npc_cmd_give_herbalist_rejects_wrong_item(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 314u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_REQUESTED;
    game_inv_bag_add(&game, ITEM_STICK);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_cmd_give(&game, ITEM_STICK, &out));
    ASSERT_EQ(HERBALIST_STORY_REQUESTED, game.herbalist_story);
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_STICK));
    ASSERT_EQ(HERBALIST_SCENE_GIVE_REJECTED, out.events[0].arg3);
    PASS();
}

TEST npc_cmd_give_herbalist_drops_reward_when_bag_full(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 315u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_REQUESTED;
    game.marsh_root_spawned = 1;
    game_inv_bag_add(&game, ITEM_MARSH_ROOT);
    /* marsh-root stays carried in bag[0] while the bag stays at capacity. */
    game.bag_count = game.bag_capacity;
    game.bag[0] = ITEM_MARSH_ROOT;
    game.bag[1] = ITEM_STICK;
    game.bag[2] = ITEM_REED;
    game.bag[3] = ITEM_STONE;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_cmd_give(&game, ITEM_MARSH_ROOT, &out));
    ASSERT_EQ(HERBALIST_STORY_COMPLETE, game.herbalist_story);
    ASSERT_EQ(1, room_has_item(&game, WORLD_ROOM_ORCHARD, ITEM_SALVE));
    ASSERT_EQ(HERBALIST_SCENE_GIVE_REWARD_GROUND, out.events[0].arg3);
    PASS();
}

TEST npc_cmd_give_herbalist_keeps_root_when_no_reward_space(void)
{
    struct GameState game;
    GameEventQueue out;
    int slot;

    unit_game_fresh(&game, 316u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_REQUESTED;
    game.marsh_root_spawned = 1;
    game_inv_bag_add(&game, ITEM_MARSH_ROOT);
    /* marsh-root stays carried while the bag and orchard ground are both full. */
    game.bag_count = game.bag_capacity;
    game.bag[0] = ITEM_MARSH_ROOT;
    game.bag[1] = ITEM_STICK;
    game.bag[2] = ITEM_REED;
    game.bag[3] = ITEM_STONE;
    game.bag[4] = ITEM_BERRY;
    for (slot = 0; slot < CFG_AREA_ITEM_SLOTS; ++slot) {
        game.room_item[WORLD_ROOM_ORCHARD][slot] = ITEM_HERB;
    }
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_cmd_give(&game, ITEM_MARSH_ROOT, &out));
    ASSERT_EQ(HERBALIST_STORY_REQUESTED, game.herbalist_story);
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_MARSH_ROOT));
    ASSERT_EQ(0, room_has_item(&game, WORLD_ROOM_ORCHARD, ITEM_SALVE));
    ASSERT_EQ(HERBALIST_SCENE_GIVE_REWARD_NO_SPACE, out.events[0].arg3);
    PASS();
}

/* Reply follows game.dialogue, not player room, after a mid-branch move. */
TEST npc_room_cmd_reply_frog_uses_dialogue_table(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 31u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_open_room_dialogue(&game, &out));
    game.player.room_id = WORLD_ROOM_CAMP;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_room_cmd_reply(&game, 2, &out));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_FROG, out.events[0].arg0);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_REPLY, out.events[0].arg1);
    ASSERT_EQ(2, out.events[0].arg2);
    PASS();
}

/* Traveler dialogue_kind is outside NPC_ROOM_INFO; reply helper is a no-op. */
TEST npc_room_cmd_reply_skips_non_room_dialogue(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 32u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_TRAVELER);
    game_event_queue_reset(&out);
    ASSERT_EQ(0, npc_room_cmd_reply(&game, 1, &out));
    ASSERT_EQ(DIALOGUE_TRAVELER, game.dialogue);
    ASSERT_EQ(0, out.count);
    PASS();
}

TEST npc_open_room_dialogue_none(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 32u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_event_queue_reset(&out);
    ASSERT_EQ(0, npc_open_room_dialogue(&game, &out));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(0, out.count);
    PASS();
}

/* Out-taking helpers assert #160 encounter/dialogue events from npc.c roaming. */
static void begin_roaming_npc(struct GameState *game, GameEventQueue *out)
{
    game_event_queue_reset(out);
    npc_roaming_begin_encounter(game, out);
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

static struct NpcState *bandit_npc(struct GameState *game)
{
    int slot;

    slot = npc_find_by_actor(game, GAME_DIALOGUE_ACTOR_BANDIT);
    if (slot < 0) {
        return 0;
    }
    return &game->npcs[slot];
}

static struct NpcState *bandit_npc_by_actor(struct GameState *game, int actor)
{
    int slot;

    slot = npc_find_by_actor(game, actor);
    if (slot < 0) {
        return 0;
    }
    return &game->npcs[slot];
}

static int profile_bandit_level(u32 seed, int actor, int room_id)
{
    /* Mirror npc_roll_profile_level salt: seed + actor + room_id. */
    return PROFILE_BANDIT_LEVEL_MIN +
        (int)((seed + (u32)actor + (u32)room_id) %
            (PROFILE_BANDIT_LEVEL_MAX - PROFILE_BANDIT_LEVEL_MIN + 1));
}


static int roaming_npc_reply_out(struct GameState *game, int choice,
                                 GameEventQueue *out)
{
    game_event_queue_reset(out);
    return npc_roaming_cmd_reply(game, choice, out);
}

TEST npc_seed_profiles_traveler_state(void)
{
    struct GameState game;
    struct NpcState *traveler;

    unit_game_fresh(&game, 39u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    traveler = traveler_npc(&game);
    ASSERT(traveler != 0);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_TRAVELER, traveler->actor);
    ASSERT_EQ(DIALOGUE_TRAVELER, traveler->dialogue);
    ASSERT_EQ(GAME_ENCOUNTER_TRAVELER, traveler->encounter);
    ASSERT_EQ(WORLD_ROOM_RUINS, traveler->room_id);
    ASSERT_EQ(NPC_FLAG_ACTIVE | NPC_FLAG_ROAMING | NPC_FLAG_RESPAWNS,
        traveler->flags);
    ASSERT_EQ(0, traveler->return_tick);
    PASS();
}

TEST npc_seed_roaming_bandit_sets_state(void)
{
    struct GameState game;
    struct NpcState *bandit;
    struct NpcState *bridge_bandit;
    struct NpcState *canyon_bandit;

    unit_game_fresh(&game, 39u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    bandit = bandit_npc(&game);
    bridge_bandit = bandit_npc_by_actor(&game, GAME_DIALOGUE_ACTOR_BANDIT_BRIDGE);
    canyon_bandit = bandit_npc_by_actor(&game, GAME_DIALOGUE_ACTOR_BANDIT_CANYON);
    ASSERT(bandit != 0);
    ASSERT(bridge_bandit != 0);
    ASSERT(canyon_bandit != 0);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_BANDIT, bandit->actor);
    ASSERT_EQ(DIALOGUE_NONE, bandit->dialogue);
    ASSERT_EQ(GAME_ENCOUNTER_BANDIT, bandit->encounter);
    ASSERT_EQ(profile_bandit_level(39u, GAME_DIALOGUE_ACTOR_BANDIT,
            WORLD_ROOM_ROAD), bandit->level);
    ASSERT_EQ(WORLD_ROOM_ROAD, bandit->room_id);
    ASSERT_EQ(NPC_FLAG_ACTIVE | NPC_FLAG_ROAMING | NPC_FLAG_RESPAWNS,
        bandit->flags);
    ASSERT_EQ(PROFILE_BANDIT_BRIDGE_SPAWN, bridge_bandit->room_id);
    ASSERT_EQ(profile_bandit_level(39u, GAME_DIALOGUE_ACTOR_BANDIT_BRIDGE,
            PROFILE_BANDIT_BRIDGE_SPAWN), bridge_bandit->level);
    ASSERT_EQ(PROFILE_BANDIT_CANYON_SPAWN, canyon_bandit->room_id);
    ASSERT_EQ(profile_bandit_level(39u, GAME_DIALOGUE_ACTOR_BANDIT_CANYON,
            PROFILE_BANDIT_CANYON_SPAWN), canyon_bandit->level);
    ASSERT_EQ(0, bandit->return_tick);
    PASS();
}

TEST npc_roaming_separation_clears(void)
{
    struct GameState game;
    struct NpcState *traveler;

    unit_game_fresh(&game, 40u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    traveler = traveler_npc(&game);
    ASSERT(traveler != 0);
    traveler->room_id = WORLD_ROOM_ROAD;
    traveler->flags |= NPC_FLAG_NEEDS_SEPARATION;
    npc_roaming_update_separation(&game);
    ASSERT_EQ(0, traveler->flags & NPC_FLAG_NEEDS_SEPARATION);
    PASS();
}

TEST npc_roaming_step_moves(void)
{
    struct GameState game;
    struct NpcState *traveler;
    struct NpcState *bandit;
    int before;

    unit_game_fresh(&game, 41u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    traveler = traveler_npc(&game);
    bandit = bandit_npc(&game);
    ASSERT(traveler != 0);
    ASSERT(bandit != 0);
    traveler->room_id = WORLD_ROOM_CAMP;
    /* Bandit stays inactive so roaming_step RNG applies only to the traveler. */
    bandit->flags &= ~NPC_FLAG_ACTIVE;
    plat_seed_rng(42u);
    ASSERT_EQ(0U, plat_rand_draw_count());
    before = traveler->room_id;
    npc_roaming_step(&game);
    ASSERT_EQ(1U, plat_rand_draw_count());
    ASSERT_NEQ(before, traveler->room_id);
    PASS();
}

TEST npc_spawn_and_presence_support_multiple_instances(void)
{
    struct GameState game;
    int slot;

    unit_game_fresh(&game, 41u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    slot = npc_spawn(&game, GAME_DIALOGUE_ACTOR_NOBODY, DIALOGUE_NONE,
        GAME_ENCOUNTER_NONE, WORLD_ROOM_MEADOW, NPC_FLAG_ACTIVE);
    ASSERT(slot >= 0);
    ASSERT_NEQ(slot, npc_find_by_actor(&game, GAME_DIALOGUE_ACTOR_TRAVELER));
    ASSERT_EQ(slot, npc_find_in_room(&game, WORLD_ROOM_MEADOW));
    ASSERT_EQ(1, npc_is_present(&game, GAME_DIALOGUE_ACTOR_NOBODY,
        WORLD_ROOM_MEADOW));
    ASSERT_EQ(slot, npc_move(&game, GAME_DIALOGUE_ACTOR_NOBODY, WORLD_ROOM_ROAD));
    ASSERT_EQ(1, npc_is_present(&game, GAME_DIALOGUE_ACTOR_NOBODY,
        WORLD_ROOM_ROAD));
    PASS();
}

TEST npc_begin_and_end_dynamic_encounter_reuses_roster(void)
{
    struct GameState game;
    GameEventQueue out;
    int slot;

    unit_game_fresh(&game, 47u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_event_queue_reset(&out);
    slot = npc_begin_encounter(&game, GAME_DIALOGUE_ACTOR_BANDIT,
        DIALOGUE_ENEMY, GAME_ENCOUNTER_BANDIT, WORLD_ROOM_CAMP, 0, &out);
    ASSERT(slot >= 0);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_ENEMY, game.dialogue);
    ASSERT_EQ(slot, npc_find_by_dialogue(&game, DIALOGUE_ENEMY));
    ASSERT_EQ(1, npc_is_present(&game, GAME_DIALOGUE_ACTOR_BANDIT,
        WORLD_ROOM_CAMP));
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_BANDIT, out.events[0].arg0);
    ASSERT_EQ(slot, npc_end_encounter(&game, GAME_DIALOGUE_ACTOR_BANDIT));
    ASSERT(game.npcs[slot].level >= PROFILE_BANDIT_LEVEL_MIN);
    ASSERT(game.npcs[slot].level <= PROFILE_BANDIT_LEVEL_MAX);
    ASSERT_EQ(-1, game.npcs[slot].room_id);
    ASSERT_EQ(0, game.npcs[slot].flags & NPC_FLAG_ACTIVE);
    PASS();
}

TEST npc_bandit_roaming_encounter_opens_in_matching_room(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *bandit;

    unit_game_fresh(&game, 47u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ROAD, 0);
    bandit = bandit_npc(&game);
    ASSERT(bandit != 0);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_roaming_begin_encounter_in_room(&game, WORLD_ROOM_ROAD, &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_ENEMY, game.dialogue);
    ASSERT_EQ(DIALOGUE_ENEMY, bandit->dialogue);
    ASSERT_EQ(NPC_FLAG_NEEDS_SEPARATION,
        bandit->flags & NPC_FLAG_NEEDS_SEPARATION);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_BANDIT, out.events[0].arg0);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_OPEN, out.events[0].arg1);
    ASSERT_EQ(bandit->level, out.events[0].arg3);
    PASS();
}

TEST npc_bandit_roaming_encounter_skips_other_rooms(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 48u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_event_queue_reset(&out);
    ASSERT_EQ(0, npc_roaming_begin_encounter_in_room(&game, WORLD_ROOM_CAMP, &out));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(0, out.count);
    PASS();
}

TEST npc_bandit_end_encounter_schedules_respawn(void)
{
    struct GameState game;
    struct NpcState *bandit;

    unit_game_fresh(&game, 49u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ROAD, 12U);
    bandit = bandit_npc(&game);
    ASSERT(bandit != 0);
    bandit->dialogue = DIALOGUE_ENEMY;
    bandit->flags |= NPC_FLAG_HANDOVER_PICK;
    plat_seed_rng(game.seed);
    ASSERT_EQ(npc_find_by_actor(&game, GAME_DIALOGUE_ACTOR_BANDIT),
        npc_end_encounter(&game, GAME_DIALOGUE_ACTOR_BANDIT));
    ASSERT_EQ(1U, plat_rand_draw_count());
    ASSERT_EQ(DIALOGUE_NONE, bandit->dialogue);
    ASSERT_EQ(0, bandit->flags & NPC_FLAG_ACTIVE);
    ASSERT_EQ(-1, bandit->room_id);
    ASSERT(bandit->return_tick >= 12U + PROFILE_BANDIT_RETURN_BASE);
    ASSERT(bandit->return_tick <
        12U + PROFILE_BANDIT_RETURN_BASE + PROFILE_BANDIT_RETURN_SPREAD);
    PASS();
}

TEST npc_bandit_activate_due_reuses_roaming_profile(void)
{
    struct GameState game;
    struct NpcState *bandit;

    unit_game_fresh(&game, 50u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 20U);
    bandit = bandit_npc(&game);
    ASSERT(bandit != 0);
    bandit->flags &= ~NPC_FLAG_ACTIVE;
    bandit->room_id = -1;
    bandit->return_tick = 20U;
    plat_seed_rng(game.seed);
    npc_roaming_activate_due(&game);
    ASSERT_EQ(1U, plat_rand_draw_count());
    ASSERT_EQ(DIALOGUE_NONE, bandit->dialogue);
    ASSERT_EQ(NPC_FLAG_ACTIVE, bandit->flags & NPC_FLAG_ACTIVE);
    ASSERT_EQ(NPC_FLAG_ROAMING, bandit->flags & NPC_FLAG_ROAMING);
    ASSERT_EQ(NPC_FLAG_RESPAWNS, bandit->flags & NPC_FLAG_RESPAWNS);
    ASSERT(bandit->level >= PROFILE_BANDIT_LEVEL_MIN);
    ASSERT(bandit->level <= PROFILE_BANDIT_LEVEL_MAX);
    ASSERT(bandit->room_id >= 0);
    ASSERT(bandit->room_id < game.world.room_count);
    PASS();
}

TEST npc_bandit_roaming_waits_for_warmup_tick(void)
{
    struct GameState game;
    struct NpcState *bandit;
    int before;

    unit_game_fresh(&game, 51u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, PROFILE_BANDIT_ROAM_START - 1);
    bandit = bandit_npc(&game);
    ASSERT(bandit != 0);
    before = bandit->room_id;
    plat_seed_rng(game.seed);
    npc_roaming_step(&game);
    ASSERT_EQ(1U, plat_rand_draw_count());
    ASSERT_EQ(before, bandit->room_id);
    PASS();
}

TEST npc_roaming_encounter_guards(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *traveler;

    unit_game_fresh(&game, 42u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    traveler = traveler_npc(&game);
    ASSERT(traveler != 0);
    traveler->flags |= NPC_FLAG_NEEDS_SEPARATION;
    begin_roaming_npc(&game, &out);
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);

    traveler->flags &= ~NPC_FLAG_NEEDS_SEPARATION;
    traveler->room_id = game.player.room_id;
    begin_roaming_npc(&game, &out);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_TRAVELER, game.dialogue);
    PASS();
}

TEST npc_roaming_reply_cmd_explore(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *traveler;

    unit_game_fresh(&game, 43u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_TRAVELER);
    traveler = traveler_npc(&game);
    ASSERT(traveler != 0);
    plat_seed_rng(game.seed);
    ASSERT_EQ(1, roaming_npc_reply_out(&game, 2, &out));
    ASSERT_EQ(1U, plat_rand_draw_count());
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(0, traveler->flags & NPC_FLAG_ACTIVE);
    ASSERT_EQ(-1, traveler->room_id);
    PASS();
}

TEST npc_roaming_reply_cmd_invalid_choice(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *traveler;

    unit_game_fresh(&game, 44u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_TRAVELER);
    traveler = traveler_npc(&game);
    ASSERT(traveler != 0);
    plat_seed_rng(game.seed);
    /* Guard path must not schedule return RNG or clear encounter state. */
    ASSERT_EQ(1, roaming_npc_reply_out(&game, 0, &out));
    ASSERT_EQ(0U, plat_rand_draw_count());
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(NPC_FLAG_ACTIVE, traveler->flags & NPC_FLAG_ACTIVE);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_PICK_123, out.events[0].arg0);
    PASS();
}

TEST npc_roaming_encounter_event(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *traveler;

    unit_game_fresh(&game, 45u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    traveler = traveler_npc(&game);
    ASSERT(traveler != 0);
    traveler->flags &= ~NPC_FLAG_NEEDS_SEPARATION;
    traveler->room_id = game.player.room_id;
    begin_roaming_npc(&game, &out);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_TRAVELER, out.events[0].arg0);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_OPEN, out.events[0].arg1);
    ASSERT_EQ(0, out.events[0].arg3);
    PASS();
}

TEST npc_roaming_reply_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 46u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_TRAVELER);
    ASSERT_EQ(1, roaming_npc_reply_out(&game, 2, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_TRAVELER, out.events[0].arg0);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_REPLY, out.events[0].arg1);
    ASSERT_EQ(2, out.events[0].arg2);
    PASS();
}

SUITE(npc) {
    RUN_TEST(npc_room_actor_lookup);
    RUN_TEST(npc_dialogue_actor_lookup);
    RUN_TEST(npc_choice_validation);
    RUN_TEST(npc_open_room_dialogue_frog);
    RUN_TEST(npc_open_room_dialogue_watchman);
    RUN_TEST(npc_open_room_dialogue_watchman_always_neutral_entry);
    RUN_TEST(npc_room_cmd_reply_watchman_meal_give_fed);
    RUN_TEST(npc_room_cmd_reply_watchman_apologize_sets_promised);
    RUN_TEST(npc_room_cmd_reply_watchman_meal_no_food);
    RUN_TEST(npc_open_room_dialogue_herbalist_requested_scene);
    RUN_TEST(npc_room_cmd_reply_herbalist_requested_root_opens_quest_submenu);
    RUN_TEST(npc_room_cmd_reply_herbalist_requested_root_gossip_leaves_dialogue);
    RUN_TEST(npc_open_room_dialogue_herbalist_reseeds_missing_root);
    RUN_TEST(npc_open_room_dialogue_herbalist_ready_scene);
    RUN_TEST(npc_room_cmd_reply_herbalist_turn_in_updates_story);
    RUN_TEST(npc_cmd_give_herbalist_rejects_before_request);
    RUN_TEST(npc_cmd_give_herbalist_rejects_wrong_item);
    RUN_TEST(npc_cmd_give_herbalist_drops_reward_when_bag_full);
    RUN_TEST(npc_cmd_give_herbalist_keeps_root_when_no_reward_space);
    RUN_TEST(npc_room_cmd_reply_frog_uses_dialogue_table);
    RUN_TEST(npc_room_cmd_reply_skips_non_room_dialogue);
    RUN_TEST(npc_open_room_dialogue_none);
    RUN_TEST(npc_seed_profiles_traveler_state);
    RUN_TEST(npc_seed_roaming_bandit_sets_state);
    RUN_TEST(npc_roaming_separation_clears);
    RUN_TEST(npc_roaming_step_moves);
    RUN_TEST(npc_spawn_and_presence_support_multiple_instances);
    RUN_TEST(npc_begin_and_end_dynamic_encounter_reuses_roster);
    RUN_TEST(npc_bandit_roaming_encounter_opens_in_matching_room);
    RUN_TEST(npc_bandit_roaming_encounter_skips_other_rooms);
    RUN_TEST(npc_bandit_end_encounter_schedules_respawn);
    RUN_TEST(npc_bandit_activate_due_reuses_roaming_profile);
    RUN_TEST(npc_bandit_roaming_waits_for_warmup_tick);
    RUN_TEST(npc_roaming_encounter_guards);
    RUN_TEST(npc_roaming_reply_cmd_explore);
    RUN_TEST(npc_roaming_reply_cmd_invalid_choice);
    RUN_TEST(npc_roaming_encounter_event);
    RUN_TEST(npc_roaming_reply_event);
}
