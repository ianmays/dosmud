#ifdef TEST_MODE

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "game.h"
#include "genc.h"
#include "grendr.h"
#include "invent.h"
#include "items.h"
#include "combat.h"
#include "npc.h"
#include "world.h"
#include "testharn.h"
#include "th_world.h"

/*
 * Harness fixtures compose real gameplay APIs into stable starting states so
 * regression snapshots exercise actual state transitions instead of mocks.
 */

static void camp_clear_ground(struct GameState *game)
{
    int s;

    for (s = 0; s < CFG_AREA_ITEM_SLOTS; ++s) {
        game->room_item[WORLD_ROOM_CAMP][s] = ITEM_NONE;
    }
}

static void harness_drop_output(GameEventQueue *out)
{
    game_event_queue_reset(out);
}

/*
 * Defers traveler respawn without freeing the roster slot; quiet fixtures rely
 * on npc_deactivate_until so roaming ticks stay inert.
 */
static void fixture_traveler_off(struct GameState *game)
{
    npc_deactivate_until(game, GAME_DIALOGUE_ACTOR_TRAVELER, 999999UL);
}

static void fixture_quiet_ticks_on(struct GameState *game)
{
    game->test_quiet_ticks = 1;
    fixture_traveler_off(game);
}

static void fixture_bandit_base(struct GameState *game)
{
    /* Most bandit fixtures start from the same room and clean ground state. */
    game_reset_fixture_baseline(game, WORLD_ROOM_CAMP, 1);
    camp_clear_ground(game);
}

static int fixture_bandit_dialogue(struct GameState *game)
{
    GameEventQueue out;

    fixture_bandit_base(game);
    if (!game_inv_bag_add(game, ITEM_STICK)) {
        return 0;
    }
    harness_drop_output(&out);
    enemy_begin_encounter(game, &out);
    game_render_output(game, &out);
    return 1;
}

static void fixture_bandit_dialogue_empty(struct GameState *game)
{
    GameEventQueue out;

    fixture_bandit_base(game);
    harness_drop_output(&out);
    enemy_begin_encounter(game, &out);
    game_render_output(game, &out);
}

static int fixture_bandit_handover_pick(struct GameState *game)
{
    int slot;

    if (!fixture_bandit_dialogue(game)) {
        return 0;
    }
    slot = npc_find_by_actor(game, GAME_DIALOGUE_ACTOR_BANDIT);
    if (slot < 0) {
        return 0;
    }
    /* slot flag is authoritative; mirror matches legacy save field for snapshots */
    game->npcs[slot].flags |= NPC_FLAG_HANDOVER_PICK;
    game->enemy_handover_pick = 1;
    render_bandit_handover_pick_prompt();
    return 1;
}

static void fixture_bandit_wielded_pick(struct GameState *game)
{
    GameEventQueue out;
    int slot;

    fixture_bandit_base(game);
    game->weapon_equipped = ITEM_STICK;
    harness_drop_output(&out);
    enemy_begin_encounter(game, &out);
    game_render_output(game, &out);
    slot = npc_find_by_actor(game, GAME_DIALOGUE_ACTOR_BANDIT);
    if (slot >= 0) {
        game->npcs[slot].flags |= NPC_FLAG_HANDOVER_PICK;
    }
    game->enemy_handover_pick = 1;
    render_bandit_handover_pick_prompt();
}

static void fixture_bandit_combat_turn1(struct GameState *game)
{
    /* The combat fixture pins the opening state so the first player reply is reproducible. */
    fixture_bandit_base(game);
    game->weapon_equipped = ITEM_STICK;
    game_set_mode_combat(game);
    game->player_hp = CFG_START_MAX_HP;
    game->combat.enemy_hp = CFG_COMBAT_ENEMY_HP_BASE;
    game->combat.defending = 0;
    render_combat_start(game->player_hp, game->combat.enemy_hp);
}

static int fixture_bandit_combat_turn1_resolve(struct GameState *game)
{
    GameEventQueue out;
    static const int equipment_rolls[2] = {
        CFG_TEST_EQUIPMENT_ROLL_PLAYER_HIT,
        CFG_TEST_EQUIPMENT_ROLL_ENEMY_DMG
    };

    fixture_bandit_combat_turn1(game);
    game_roll_inject_begin(game, equipment_rolls, 2);
    harness_drop_output(&out);
    combat_resolve_reply(game, 1, &out);
    game_render_output(game, &out);
    if (!game_roll_inject_fully_consumed(game)) {
        game_roll_inject_clear(game);
        return 0;
    }
    game_roll_inject_clear(game);
    return 1;
}

static void fixture_bandit_combat_near_kill(struct GameState *game)
{
    fixture_bandit_combat_turn1(game);
    game->combat.enemy_hp = 1;
}

static void fixture_bandit_combat_defend_ready(struct GameState *game)
{
    static const int rolls[1] = { CFG_TEST_COMBAT_DEFEND_ENEMY_DMG };

    fixture_bandit_combat_turn1(game);
    game_roll_inject_begin(game, rolls, 1);
}

static int fixture_bandit_combat_salve_ready(struct GameState *game)
{
    static const int rolls[1] = { CFG_TEST_COMBAT_SALVE_ENEMY_DMG };

    fixture_bandit_combat_turn1(game);
    if (!game_inv_bag_add(game, ITEM_SALVE)) {
        return 0;
    }
    game_roll_inject_begin(game, rolls, 1);
    return 1;
}

static void fixture_bandit_victory_inject(struct GameState *game, int loot_percent)
{
    int rolls[3];

    rolls[0] = CFG_TEST_VICTORY_HIT_SPREAD;
    rolls[1] = loot_percent;
    rolls[2] = CFG_TEST_VICTORY_XP_SPREAD;
    fixture_bandit_combat_near_kill(game);
    game_roll_inject_begin(game, rolls, 3);
}

static void fixture_bandit_combat_level_ready(struct GameState *game)
{
    fixture_bandit_victory_inject(game, CFG_TEST_VICTORY_LOOT_STICK);
    game->xp = 19;
}

static int fixture_bandit_intimidate_inject(struct GameState *game, int roll)
{
    int rolls[1];

    if (!fixture_bandit_dialogue(game)) {
        return 0;
    }
    rolls[0] = roll;
    game_roll_inject_begin(game, rolls, 1);
    return 1;
}

static int fixture_bandit_fight_ready(struct GameState *game)
{
    static const int rolls[1] = { CFG_TEST_FIGHT_ENEMY_HP_SPREAD };

    if (!fixture_bandit_dialogue(game)) {
        return 0;
    }
    game_roll_inject_begin(game, rolls, 1);
    return 1;
}

static void fixture_world_boot(struct GameState *game)
{
    harness_world_boot_graph(game);
}

static void fixture_at_camp(struct GameState *game)
{
    game_reset_fixture_baseline(game, WORLD_ROOM_CAMP, 0);
    camp_clear_ground(game);
    game->room_explored[WORLD_ROOM_CAMP] = 1;
}

static void fixture_at_road(struct GameState *game)
{
    /* Road fixtures mark only the rooms needed by the snapshot path. */
    game_reset_fixture_baseline(game, WORLD_ROOM_ROAD, 1);
    game->room_explored[WORLD_ROOM_CAMP] = 1;
    game->room_explored[WORLD_ROOM_ROAD] = 1;
}

/* Road + inactive traveler so wait ticks hit the fixed bandit, not roaming overlap. */
static void fixture_fixed_bandit_road(struct GameState *game)
{
    fixture_at_road(game);
    fixture_traveler_off(game);
}

static int fixture_at_marsh_reed(struct GameState *game)
{
    game_reset_fixture_baseline(game, WORLD_ROOM_MARSH, 2);
    camp_clear_ground(game);
    if (!game_inv_bag_add(game, ITEM_STICK)) {
        return 0;
    }
    game->room_item[WORLD_ROOM_MARSH][0] = ITEM_REED;
    game->room_explored[WORLD_ROOM_CAMP] = 1;
    game->room_explored[WORLD_ROOM_MARSH] = 1;
    game_set_mode_explore(game);
    return 1;
}

static void fixture_at_pond(struct GameState *game)
{
    game_reset_fixture_baseline(game, WORLD_ROOM_POND, 0);
    game->room_explored[WORLD_ROOM_POND] = 1;
}

static void fixture_at_tower(struct GameState *game)
{
    game_reset_fixture_baseline(game, WORLD_ROOM_TOWER, 0);
    game->room_explored[WORLD_ROOM_TOWER] = 1;
}

static void fixture_at_orchard(struct GameState *game)
{
    game_reset_fixture_baseline(game, WORLD_ROOM_ORCHARD, 0);
    game->room_explored[WORLD_ROOM_ORCHARD] = 1;
}

static void fixture_at_catacombs(struct GameState *game)
{
    game_reset_fixture_baseline(game, WORLD_ROOM_CATACOMBS, 0);
    game->room_explored[WORLD_ROOM_CATACOMBS] = 1;
}

static void fixture_quiet_explore(struct GameState *game)
{
    fixture_at_camp(game);
    fixture_quiet_ticks_on(game);
}

/* Ambient snapshots: camp baseline with wanderer off; ticks still emit atmosphere. */
static void fixture_ambient_camp(struct GameState *game)
{
    fixture_at_camp(game);
    fixture_traveler_off(game);
}

static void fixture_quiet_camp_dual_ground(struct GameState *game)
{
    game_reset_fixture_baseline(game, WORLD_ROOM_CAMP, 0);
    camp_clear_ground(game);
    game->room_item[WORLD_ROOM_CAMP][0] = ITEM_STICK;
    game->room_item[WORLD_ROOM_CAMP][1] = ITEM_REED;
    game->room_explored[WORLD_ROOM_CAMP] = 1;
    fixture_quiet_ticks_on(game);
}

static int fixture_quiet_camp_dual_ground_full_bag(struct GameState *game)
{
    int i;

    fixture_quiet_camp_dual_ground(game);
    for (i = 0; i < game->bag_capacity; ++i) {
        if (!game_inv_bag_add(game, ITEM_BERRY)) {
            return 0;
        }
    }
    return 1;
}

static void fixture_traveler_dialogue(struct GameState *game)
{
    GameEventQueue out;
    int slot;

    game_reset_fixture_baseline(game, WORLD_ROOM_ROAD, 0);
    game->room_explored[WORLD_ROOM_ROAD] = 1;
    slot = npc_find_by_actor(game, GAME_DIALOGUE_ACTOR_TRAVELER);
    game->npcs[slot].flags |= NPC_FLAG_ACTIVE;
    game->npcs[slot].flags &= ~NPC_FLAG_NEEDS_SEPARATION;
    game->npcs[slot].room_id = WORLD_ROOM_ROAD;
    harness_drop_output(&out);
    npc_roaming_begin_encounter(game, &out);
    game_render_output(game, &out);
}

static int fixture_bag_item(struct GameState *game, int room_id, int item_id)
{
    game_reset_fixture_baseline(game, room_id, 0);
    game->room_explored[room_id] = 1;
    if (!game_inv_bag_add(game, item_id)) {
        return 0;
    }
    return 1;
}

static int fixture_bag_berry_low_hp(struct GameState *game)
{
    if (!fixture_bag_item(game, WORLD_ROOM_CAMP, ITEM_BERRY)) {
        return 0;
    }
    game->player_hp = CFG_START_MAX_HP - 5;
    return 1;
}

static int fixture_bag_stacked(struct GameState *game)
{
    game_reset_fixture_baseline(game, WORLD_ROOM_CAMP, 0);
    game->room_explored[WORLD_ROOM_CAMP] = 1;
    if (!game_inv_bag_add(game, ITEM_BERRY)) {
        return 0;
    }
    if (!game_inv_bag_add(game, ITEM_BERRY)) {
        return 0;
    }
    if (!game_inv_bag_add(game, ITEM_STICK)) {
        return 0;
    }
    return 1;
}

static int fixture_bag_fish_low_hp(struct GameState *game)
{
    if (!fixture_bag_item(game, WORLD_ROOM_POND, ITEM_FISH)) {
        return 0;
    }
    game->player_hp = CFG_START_MAX_HP - 5;
    return 1;
}

static void fixture_env_focus(struct GameState *game, int kind)
{
    fixture_at_camp(game);
    game->env_focus_active = 1;
    game->env_focus_room = game->player.room_id;
    game->env_focus_kind = kind;
    game->env_focus_expires_tick = game->tick + CFG_ENV_FOCUS_DURATION_TICKS;
}

static void fixture_corpse_stripped(struct GameState *game)
{
    fixture_at_camp(game);
    game->corpse_present[WORLD_ROOM_CAMP] = 1;
    game->corpse_loot[WORLD_ROOM_CAMP] = ITEM_NONE;
}

static int fixture_corpse_loot_full_bag(struct GameState *game, int loot_item)
{
    int i;

    fixture_at_camp(game);
    for (i = 0; i < game->bag_capacity; ++i) {
        if (!game_inv_bag_add(game, ITEM_BERRY)) {
            return 0;
        }
    }
    game->corpse_present[WORLD_ROOM_CAMP] = 1;
    game->corpse_loot[WORLD_ROOM_CAMP] = loot_item;
    return 1;
}

static int fixture_bag_craft_salve(struct GameState *game)
{
    fixture_at_camp(game);
    if (!game_inv_bag_add(game, ITEM_HERB)) {
        return 0;
    }
    if (!game_inv_bag_add(game, ITEM_BERRY)) {
        return 0;
    }
    return 1;
}

static int fixture_name_is(const char *name, const char *line)
{
    size_t nlen;

    nlen = strlen(name);
    if (strncmp(line, name, nlen) != 0) {
        return 0;
    }
    return line[nlen] == '\0' || line[nlen] == ' ' || line[nlen] == '\t';
}

static int parse_seed_value(const char *p, u32 *out_seed)
{
    char *end;
    unsigned long val;

    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    if (*p == '\0') {
        return -1;
    }
    if (*p == '-' || *p == '+') {
        return -1;
    }
    errno = 0;
    val = strtoul(p, &end, 10);
    if (end == p) {
        return -1;
    }
    while (*end == ' ' || *end == '\t') {
        ++end;
    }
    if (*end != '\0') {
        return -1;
    }
    if (errno == ERANGE) {
        return -1;
    }
#if ULONG_MAX > CFG_SEED_CLI_MAX
    if (val > (unsigned long)CFG_SEED_CLI_MAX) {
        return -1;
    }
#endif
    *out_seed = (u32)val;
    return 0;
}

int testharn_apply(struct GameState *game, const char *line)
{
    const char *p;
    const char *name;

    if (line == 0 || line[0] != '@') {
        return 0;
    }
    p = line + 1;
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    if (strncmp(p, "seed", 4) == 0 &&
        (p[4] == '\0' || p[4] == ' ' || p[4] == '\t')) {
        p += 4;
        if (parse_seed_value(p, &game->seed) != 0) {
            return -3;
        }
        return 1;
    }
    if (strncmp(p, "fixture", 7) != 0) {
        return 0;
    }
    p += 7;
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    if (*p == '\0') {
        return -1;
    }
    name = p;

    if (fixture_name_is("bandit_dialogue", name)) {
        if (!fixture_bandit_dialogue(game)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("bandit_dialogue_empty", name)) {
        fixture_bandit_dialogue_empty(game);
        return 1;
    }
    if (fixture_name_is("bandit_handover_pick", name)) {
        if (!fixture_bandit_handover_pick(game)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("bandit_wielded_pick", name)) {
        fixture_bandit_wielded_pick(game);
        return 1;
    }
    if (fixture_name_is("bandit_combat_turn1", name)) {
        fixture_bandit_combat_turn1(game);
        return 1;
    }
    if (fixture_name_is("bandit_combat_turn1_resolve", name)) {
        if (!fixture_bandit_combat_turn1_resolve(game)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("bandit_combat_defend_ready", name)) {
        fixture_bandit_combat_defend_ready(game);
        return 1;
    }
    if (fixture_name_is("bandit_combat_salve_ready", name)) {
        if (!fixture_bandit_combat_salve_ready(game)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("bandit_combat_level_ready", name)) {
        fixture_bandit_combat_level_ready(game);
        return 1;
    }
    if (fixture_name_is("bandit_fight_ready", name)) {
        if (!fixture_bandit_fight_ready(game)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("bandit_intimidate_ok", name)) {
        if (!fixture_bandit_intimidate_inject(game, CFG_TEST_INTIMIDATE_OK)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("bandit_intimidate_fail", name)) {
        if (!fixture_bandit_intimidate_inject(game, CFG_TEST_INTIMIDATE_FAIL)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("bandit_victory_spear", name)) {
        fixture_bandit_victory_inject(game, CFG_TEST_VICTORY_LOOT_SPEAR);
        return 1;
    }
    if (fixture_name_is("bandit_victory_stick", name)) {
        fixture_bandit_victory_inject(game, CFG_TEST_VICTORY_LOOT_STICK);
        return 1;
    }
    if (fixture_name_is("bandit_victory_berry", name)) {
        fixture_bandit_victory_inject(game, CFG_TEST_VICTORY_LOOT_BERRY);
        return 1;
    }
    if (fixture_name_is("bandit_victory_herb", name)) {
        fixture_bandit_victory_inject(game, CFG_TEST_VICTORY_LOOT_HERB);
        return 1;
    }
    if (fixture_name_is("bandit_victory_fish", name)) {
        fixture_bandit_victory_inject(game, CFG_TEST_VICTORY_LOOT_FISH);
        return 1;
    }
    if (fixture_name_is("world_boot", name)) {
        fixture_world_boot(game);
        return 1;
    }
    if (fixture_name_is("world_linear", name)) {
        fixture_world_boot(game);
        return 1;
    }
    if (fixture_name_is("at_camp", name)) {
        fixture_at_camp(game);
        return 1;
    }
    if (fixture_name_is("at_road", name)) {
        fixture_at_road(game);
        return 1;
    }
    if (fixture_name_is("fixed_bandit_road", name)) {
        fixture_fixed_bandit_road(game);
        return 1;
    }
    if (fixture_name_is("at_marsh_reed", name)) {
        if (!fixture_at_marsh_reed(game)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("at_pond", name)) {
        fixture_at_pond(game);
        return 1;
    }
    if (fixture_name_is("at_tower", name)) {
        fixture_at_tower(game);
        return 1;
    }
    if (fixture_name_is("at_orchard", name)) {
        fixture_at_orchard(game);
        return 1;
    }
    if (fixture_name_is("at_catacombs", name)) {
        fixture_at_catacombs(game);
        return 1;
    }
    if (fixture_name_is("quiet_explore", name)) {
        fixture_quiet_explore(game);
        return 1;
    }
    if (fixture_name_is("ambient_camp", name)) {
        fixture_ambient_camp(game);
        return 1;
    }
    if (fixture_name_is("quiet_camp_dual_ground", name)) {
        fixture_quiet_camp_dual_ground(game);
        return 1;
    }
    if (fixture_name_is("quiet_camp_dual_ground_full_bag", name)) {
        if (!fixture_quiet_camp_dual_ground_full_bag(game)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("traveler_dialogue", name)) {
        fixture_traveler_dialogue(game);
        return 1;
    }
    if (fixture_name_is("bag_berry", name)) {
        if (!fixture_bag_item(game, WORLD_ROOM_CAMP, ITEM_BERRY)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("bag_stacked", name)) {
        if (!fixture_bag_stacked(game)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("bag_fish", name)) {
        if (!fixture_bag_item(game, WORLD_ROOM_POND, ITEM_FISH)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("bag_berry_low_hp", name)) {
        if (!fixture_bag_berry_low_hp(game)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("bag_fish_low_hp", name)) {
        if (!fixture_bag_fish_low_hp(game)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("bag_salve", name)) {
        if (!fixture_bag_item(game, WORLD_ROOM_CAMP, ITEM_SALVE)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("bag_torch", name)) {
        if (!fixture_bag_item(game, WORLD_ROOM_CAMP, ITEM_TORCH)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("bag_spear", name)) {
        if (!fixture_bag_item(game, WORLD_ROOM_CAMP, ITEM_SPEAR)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("bag_stone", name)) {
        if (!fixture_bag_item(game, WORLD_ROOM_CAMP, ITEM_STONE)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("bag_stick", name)) {
        if (!fixture_bag_item(game, WORLD_ROOM_CAMP, ITEM_STICK)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("bag_craft_salve", name)) {
        if (!fixture_bag_craft_salve(game)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("corpse_stripped", name)) {
        fixture_corpse_stripped(game);
        return 1;
    }
    if (fixture_name_is("corpse_loot_full_bag", name)) {
        if (!fixture_corpse_loot_full_bag(game, ITEM_STICK)) {
            return -2;
        }
        return 1;
    }
    if (fixture_name_is("env_focus_rustle", name)) {
        fixture_env_focus(game, GAME_ENV_RUSTLE);
        return 1;
    }
    if (fixture_name_is("env_focus_creak", name)) {
        fixture_env_focus(game, GAME_ENV_CREAK);
        return 1;
    }
    if (fixture_name_is("env_focus_water", name)) {
        fixture_env_focus(game, GAME_ENV_WATER);
        return 1;
    }
    if (fixture_name_is("env_focus_grit", name)) {
        fixture_env_focus(game, GAME_ENV_GRIT);
        return 1;
    }
    if (fixture_name_is("bag_full_gate", name)) {
        if (!game_inv_bag_add(game, ITEM_STICK)) {
            return -2;
        }
        return 1;
    }
    return -1;
}

#endif
