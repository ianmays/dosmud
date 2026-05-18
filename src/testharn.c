#ifdef TEST_MODE

#include <string.h>
#include "config.h"
#include "game.h"
#include "genc.h"
#include "grendr.h"
#include "invent.h"
#include "items.h"
#include "testharn.h"

static void camp_clear_ground(struct GameState *game)
{
    int s;

    for (s = 0; s < CFG_AREA_ITEM_SLOTS; ++s) {
        game->room_item[WORLD_ROOM_CAMP][s] = ITEM_NONE;
    }
}

static void fixture_bandit_base(struct GameState *game)
{
    game_reset_fixture_baseline(game, WORLD_ROOM_CAMP, 1);
    camp_clear_ground(game);
}

static int fixture_bandit_dialogue(struct GameState *game)
{
    fixture_bandit_base(game);
    if (!game_inv_bag_add(game, ITEM_STICK)) {
        return 0;
    }
    enemy_begin_encounter(game);
    return 1;
}

static int fixture_bandit_handover_pick(struct GameState *game)
{
    if (!fixture_bandit_dialogue(game)) {
        return 0;
    }
    game->enemy_handover_pick = 1;
    render_bandit_handover_pick_prompt();
    return 1;
}

static void fixture_bandit_wielded_pick(struct GameState *game)
{
    fixture_bandit_base(game);
    game->weapon_equipped = ITEM_STICK;
    enemy_begin_encounter(game);
    game->enemy_handover_pick = 1;
    render_bandit_handover_pick_prompt();
}

static int fixture_bandit_combat_turn1(struct GameState *game)
{
    fixture_bandit_base(game);
    game->weapon_equipped = ITEM_STICK;
    game_set_mode_combat(game);
    game->player_hp = CFG_START_MAX_HP;
    game->combat.enemy_hp = 8;
    game->combat.defending = 0;
    render_combat_start(game->player_hp, game->combat.enemy_hp);
    return 1;
}

static void fixture_at_camp(struct GameState *game)
{
    game_reset_fixture_baseline(game, WORLD_ROOM_CAMP, 0);
    camp_clear_ground(game);
    game->room_explored[WORLD_ROOM_CAMP] = 1;
}

static void fixture_at_road(struct GameState *game)
{
    game_reset_fixture_baseline(game, WORLD_ROOM_ROAD, 1);
    game->room_explored[WORLD_ROOM_CAMP] = 1;
    game->room_explored[WORLD_ROOM_ROAD] = 1;
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

static int fixture_name_is(const char *name, const char *line)
{
    size_t nlen;

    nlen = strlen(name);
    if (strncmp(line, name, nlen) != 0) {
        return 0;
    }
    return line[nlen] == '\0' || line[nlen] == ' ' || line[nlen] == '\t';
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
        if (!fixture_bandit_combat_turn1(game)) {
            return -2;
        }
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
    if (fixture_name_is("at_marsh_reed", name)) {
        if (!fixture_at_marsh_reed(game)) {
            return -2;
        }
        return 1;
    }
    return -1;
}

#endif
