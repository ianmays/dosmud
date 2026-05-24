#include <stdio.h>
#include <stdarg.h>
#include "grendr.h"
#include "game.h"
#include "combat.h"
#include "items.h"
#include "command.h"
#include "world.h"
#include "txtres.h"
#include "fmt.h"

#ifdef TEST_MODE
static int g_render_suppress;

void render_set_suppress(int on)
{
    g_render_suppress = on ? 1 : 0;
}
#endif

static void render_emit(const char *fmt, ...)
{
    va_list ap;

#ifdef TEST_MODE
    if (g_render_suppress) {
        return;
    }
#endif
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

#define RENDER_PRINTF render_emit

/*
 * Newline tiers: txtres strings end with \n only; grendr owns gaps.
 * render_gap = scene/paragraph break; render_paragraph = gap + copy.
 */
static void render_gap(void)
{
    RENDER_PRINTF("\n");
}

static void render_copy(const char *text)
{
    RENDER_PRINTF("%s", text);
}

static void render_paragraph(const char *text)
{
    render_gap();
    render_copy(text);
}

static void art_room_camp(void)
{
    RENDER_PRINTF("       *      *        $   * \n");
    RENDER_PRINTF("   *               *   $     \n");
    RENDER_PRINTF("      _________        $     \n");
    RENDER_PRINTF("     /       /|\\       $    \n");
    RENDER_PRINTF("    /       / | \\     \\|/  \n");
    RENDER_PRINTF("___/_______/__|__\\ ___XXX___\n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF(" %s", g_room_art_captions[WORLD_ROOM_CAMP]);
    RENDER_PRINTF("                             \n");
}

static void art_room_road(void)
{
    RENDER_PRINTF("  ##     #####      #####    \n");
    RENDER_PRINTF("        ##########          #\n");
    RENDER_PRINTF("   ####         ###          \n");
    RENDER_PRINTF("_____________________________\n");
    RENDER_PRINTF("            /    \\          \n");
    RENDER_PRINTF("           /      \\         \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("   %s", g_room_art_captions[WORLD_ROOM_ROAD]);
    RENDER_PRINTF("                             \n");
}

static void art_room_pond(void)
{
    RENDER_PRINTF(" |  | || |  ||  ||| |    | | \n");
    RENDER_PRINTF("|_||||||_|||||__|||||||_||_||\n");
    RENDER_PRINTF("   ^   __|_____||___|_       \n");
    RENDER_PRINTF("     |/    o       O  \\  ^  \n");
    RENDER_PRINTF("     (        O   |    )|    \n");
    RENDER_PRINTF(" ^    \\__||______o_|__/     \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("  %s", g_room_art_captions[WORLD_ROOM_POND]);
    RENDER_PRINTF("                             \n");
}

static void art_room_forest(void)
{
    RENDER_PRINTF("       &&     &&     &&      \n");
    RENDER_PRINTF("      &/\\&   &/\\&   &/\\&  \n");
    RENDER_PRINTF("     &|()|& &|()|& &|()|&    \n");
    RENDER_PRINTF("      \\||/   \\||/   \\||/  \n");
    RENDER_PRINTF("_______||_____||_____||______\n");
    RENDER_PRINTF("    ^^       ^^^^     ^^^    \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("%s", g_room_art_captions[WORLD_ROOM_FOREST]);
    RENDER_PRINTF("                             \n");
}

static void art_room_stream(void)
{
    RENDER_PRINTF("____&________________________\n");
    RENDER_PRINTF("   ooo      &           _/   \n");
    RENDER_PRINTF("    |   &  ooo         /__   \n");
    RENDER_PRINTF("       ooo  |         ____\\ \n");
    RENDER_PRINTF("        |        ___/        \n");
    RENDER_PRINTF("________________/            \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("%s", g_room_art_captions[WORLD_ROOM_STREAM]);
    RENDER_PRINTF("                             \n");
}

static void art_room_ruins(void)
{
    RENDER_PRINTF(" #####        ########       \n");
    RENDER_PRINTF("               #####         \n");
    RENDER_PRINTF("     |\\              /|     \n");
    RENDER_PRINTF("     | \\__.--''--.__/ |     \n");
    RENDER_PRINTF(" ____|_|_|________|_|_|____  \n");
    RENDER_PRINTF("/__________________________\\\n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF(" %s", g_room_art_captions[WORLD_ROOM_RUINS]);
    RENDER_PRINTF("                             \n");
}

static void art_room_cliff(void)
{
    RENDER_PRINTF("    #####            ####    \n");
    RENDER_PRINTF("   ###        /\\            \n");
    RENDER_PRINTF("        /\\   /  \\   /\\    \n");
    RENDER_PRINTF("       /__\\_/____\\_/__\\   \n");
    RENDER_PRINTF("      /________________\\    \n");
    RENDER_PRINTF("     /__________________\\   \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("      %s", g_room_art_captions[WORLD_ROOM_CLIFF]);
    RENDER_PRINTF("                             \n");
}

static void art_room_marsh(void)
{
    RENDER_PRINTF("   ########        ######    \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("  ||    |    ||    |    ||   \n");
    RENDER_PRINTF("  ||  .-|-.  ||  .-|-.  ||   \n");
    RENDER_PRINTF("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    RENDER_PRINTF("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("  %s", g_room_art_captions[WORLD_ROOM_MARSH]);
    RENDER_PRINTF("                             \n");
}

static void art_room_grove(void)
{
    RENDER_PRINTF("         &       &           \n");
    RENDER_PRINTF("     &  ooo  &  ooo  &       \n");
    RENDER_PRINTF("    ooo  |  ooo  |  ooo  ^   \n");
    RENDER_PRINTF("     |   &   |   &   |       \n");
    RENDER_PRINTF("  ^     ooo     ooo          \n");
    RENDER_PRINTF("         |       |     ^^    \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("   %s", g_room_art_captions[WORLD_ROOM_GROVE]);
    RENDER_PRINTF("                             \n");
}

static void art_room_bridge(void)
{
    RENDER_PRINTF("       ||\\          /||     \n");
    RENDER_PRINTF("~~~~~~~~~~|        |~~~~~~~~~\n");
    RENDER_PRINTF("~~~~~~~~~~|        |~~~~~~~~~\n");
    RENDER_PRINTF("~~~~~~~~~~|        |~~~~~~~~~\n");
    RENDER_PRINTF("~~~~~~~~~~|        |~~~~~~~~~\n");
    RENDER_PRINTF("       ||/          \\||     \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("    %s", g_room_art_captions[WORLD_ROOM_BRIDGE]);
    RENDER_PRINTF("                             \n");
}

static void art_room_catacombs(void)
{
    RENDER_PRINTF("=============================\n");
    RENDER_PRINTF("oooo ooooOOOOOooooooooOOOOooo\n");
    RENDER_PRINTF("ooooooooooooooooOOOo   oooooo\n");
    RENDER_PRINTF("oo  ooooOOooooooooooooOOOOooo\n");
    RENDER_PRINTF("oooOOOoooooooOOO OOoooooooooo\n");
    RENDER_PRINTF("ooooooooooOO oo  ooooOOOooooo\n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("   %s", g_room_art_captions[WORLD_ROOM_CATACOMBS]);
    RENDER_PRINTF("                             \n");
}

static void art_room_meadow(void)
{
    RENDER_PRINTF("    ######     #####  ###    \n");
    RENDER_PRINTF("           ##     ###        \n");
    RENDER_PRINTF("-*-*--*------*-----*-*----*--\n");
    RENDER_PRINTF("- * -- * * -- -- * * -- * ---\n");
    RENDER_PRINTF("---  @  ---  @  -------  @  -\n");
    RENDER_PRINTF("------   @   ---   @   @   --\n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("%s", g_room_art_captions[WORLD_ROOM_MEADOW]);
    RENDER_PRINTF("                             \n");
}

static void art_room_canyon(void)
{
    RENDER_PRINTF("__________      ### _________\n");
    RENDER_PRINTF("          \\ ##    |         \n");
    RENDER_PRINTF("          /________\\        \n");
    RENDER_PRINTF("           \\   \\ __/       \n");
    RENDER_PRINTF("           /  _/ \\          \n");
    RENDER_PRINTF("          /  /   /           \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("  %s", g_room_art_captions[WORLD_ROOM_CANYON]);
    RENDER_PRINTF("                             \n");
}

static void art_room_tower(void)
{
    RENDER_PRINTF("             /''\\   ####    \n");
    RENDER_PRINTF("            /____\\   ##     \n");
    RENDER_PRINTF("           | [__] |          \n");
    RENDER_PRINTF("___________|      |__________\n");
    RENDER_PRINTF("   \\|/     |  __  |         \n");
    RENDER_PRINTF("          /__|__|__\\   \\|/ \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF(" %s", g_room_art_captions[WORLD_ROOM_TOWER]);
    RENDER_PRINTF("                             \n");
}

static void art_room_orchard(void)
{
    RENDER_PRINTF("                  @@         \n");
    RENDER_PRINTF("    &   &       @.@@.@    &  \n");
    RENDER_PRINTF("    '  ooo     @@@'@@'@   '  \n");
    RENDER_PRINTF("        |     @'@.@@'@'@     \n");
    RENDER_PRINTF("   ^^           ' | o|''   ^ \n");
    RENDER_PRINTF("        ^^^     ..|  |.      \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("%s", g_room_art_captions[WORLD_ROOM_ORCHARD]);
    RENDER_PRINTF("                             \n");
}

static void art_room_cave(void)
{
    RENDER_PRINTF("'''.._----_______---__-_     \n");
    RENDER_PRINTF("                        |    \n");
    RENDER_PRINTF(" ^    ^   .-----.    ^^  '-- \n");
    RENDER_PRINTF("  {{     |       |      }    \n");
    RENDER_PRINTF("_________|_______|___________\n");
    RENDER_PRINTF("  ''' ''          '' ''''    \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF(" %s", g_room_art_captions[WORLD_ROOM_CAVE]);
    RENDER_PRINTF("                             \n");
}

static void art_wanderer(void)
{
    RENDER_PRINTF("        .-''''-.             \n");
    RENDER_PRINTF("       / o)o)   |            \n");
    RENDER_PRINTF("      |   /_    B ------     \n");
    RENDER_PRINTF("      |   ---  / //     \\   \n");
    RENDER_PRINTF("       \\______/_//       \\ \n");
    RENDER_PRINTF("      /        //\\_______/  \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF(" %s", TXT_WANDERER_ART_CAPTION);
    RENDER_PRINTF("                             \n");
}

static void art_watchman_portrait(void)
{
    RENDER_PRINTF("        .#######.            \n");
    RENDER_PRINTF("       /#########\\          \n");
    RENDER_PRINTF("       |  -- (o) |           \n");
    RENDER_PRINTF("       |    ^    |           \n");
    RENDER_PRINTF("       \\___---___/          \n");
    RENDER_PRINTF("      ----/'''\\---          \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("%s", TXT_WATCHMAN_ART_CAPTION);
    RENDER_PRINTF("                             \n");
}

static void art_herbalist_portrait(void)
{
    RENDER_PRINTF("        _;;;;;;_             \n");
    RENDER_PRINTF("       / _    _ \\           \n");
    RENDER_PRINTF("      (-( )--( )-)           \n");
    RENDER_PRINTF("       |    ^   |            \n");
    RENDER_PRINTF("        \\__ - _/            \n");
    RENDER_PRINTF("       ___|''' |___          \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("  %s", TXT_HERBALIST_ART_CAPTION);
    RENDER_PRINTF("                             \n");
}

static void art_archivist_portrait(void)
{
    RENDER_PRINTF("       /|||||||||\\          \n");
    RENDER_PRINTF("      ||/ _   _ \\||         \n");
    RENDER_PRINTF("      ||  0   0  ||          \n");
    RENDER_PRINTF("      ||    _    ||          \n");
    RENDER_PRINTF("      ||\\_/___\\_/||        \n");
    RENDER_PRINTF("     _|||_\\|||/_|||_        \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("  %s", TXT_ARCHIVIST_ART_CAPTION);
    RENDER_PRINTF("                             \n");
}

static void art_frog_portrait(void)
{
    RENDER_PRINTF("        ___---___            \n");
    RENDER_PRINTF("      (|)  . .  (|)          \n");
    RENDER_PRINTF("       \\_________/          \n");
    RENDER_PRINTF("      //          \\\\       \n");
    RENDER_PRINTF("     ||            ||        \n");
    RENDER_PRINTF("    /||\\__________/||\\     \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("%s", TXT_FROG_ART_CAPTION);
    RENDER_PRINTF("                             \n");
}

static void art_for_room(int room_id)
{
    if (room_id == WORLD_ROOM_CAMP) {
        art_room_camp();
        return;
    }
    if (room_id == WORLD_ROOM_ROAD) {
        art_room_road();
        return;
    }
    if (room_id == WORLD_ROOM_POND) {
        art_room_pond();
        return;
    }
    if (room_id == WORLD_ROOM_FOREST) {
        art_room_forest();
        return;
    }
    if (room_id == WORLD_ROOM_STREAM) {
        art_room_stream();
        return;
    }
    if (room_id == WORLD_ROOM_RUINS) {
        art_room_ruins();
        return;
    }
    if (room_id == WORLD_ROOM_CLIFF) {
        art_room_cliff();
        return;
    }
    if (room_id == WORLD_ROOM_MARSH) {
        art_room_marsh();
        return;
    }
    if (room_id == WORLD_ROOM_GROVE) {
        art_room_grove();
        return;
    }
    if (room_id == WORLD_ROOM_BRIDGE) {
        art_room_bridge();
        return;
    }
    if (room_id == WORLD_ROOM_CATACOMBS) {
        art_room_catacombs();
        return;
    }
    if (room_id == WORLD_ROOM_MEADOW) {
        art_room_meadow();
        return;
    }
    if (room_id == WORLD_ROOM_CANYON) {
        art_room_canyon();
        return;
    }
    if (room_id == WORLD_ROOM_TOWER) {
        art_room_tower();
        return;
    }
    if (room_id == WORLD_ROOM_ORCHARD) {
        art_room_orchard();
        return;
    }
    if (room_id == WORLD_ROOM_CAVE) {
        art_room_cave();
        return;
    }
}

void game_print_location_art(int room_id)
{
    render_gap();
    art_for_room(room_id);
}

void render_room_look(struct GameState *game, int npc_in_room_hint)
{
    struct Room *room;
    int dir;
    int rid;
    char ground[CFG_FMT_GROUND_MAX];
    int ground_len;

    room = &game->world.rooms[game->player.room_id];
    rid = game->player.room_id;
    game_print_location_art(game->player.room_id);
    render_gap();
    RENDER_PRINTF("%s\n", room->name);
    RENDER_PRINTF("%s\n", room->desc);
    RENDER_PRINTF("%s", TXT_UI_EXITS_LABEL);
    for (dir = 0; dir < DIR_NONE; ++dir) {
        if (room->exits[dir] >= 0) {
            RENDER_PRINTF(" %s", world_dir_name(dir));
        }
    }
    RENDER_PRINTF("\n");
    ground_len = fmt_room_ground_items(game, rid, ground, (int)sizeof(ground));
    if (ground_len > 0) {
        RENDER_PRINTF("%s", ground);
    } else if (ground_len < 0) {
        RENDER_PRINTF("%s", TXT_UI_GROUND_ITEMS_TOO_LONG);
    }
    if (game->corpse_present[game->player.room_id]) {
        RENDER_PRINTF("%s", TXT_UI_BANDIT_CORPSE);
    }
    if (npc_in_room_hint != 0) {
        RENDER_PRINTF("%s", TXT_UI_NPC_HINT);
    }
    if (game->env_focus_active &&
            game->env_focus_room == game->player.room_id &&
            game->tick < game->env_focus_expires_tick) {
        if (game->env_focus_kind == GAME_ENV_RUSTLE) {
            RENDER_PRINTF("%s", TXT_UI_FOCUS_RUSTLE);
        } else if (game->env_focus_kind == GAME_ENV_CREAK) {
            RENDER_PRINTF("%s", TXT_UI_FOCUS_CREAK);
        } else if (game->env_focus_kind == GAME_ENV_WATER) {
            RENDER_PRINTF("%s", TXT_UI_FOCUS_WATER);
        } else if (game->env_focus_kind == GAME_ENV_GRIT) {
            RENDER_PRINTF("%s", TXT_UI_FOCUS_GRIT);
        }
    }
}

void game_render(const struct GameState *game)
{
    const struct Room *room;
    int needed;

    room = &game->world.rooms[game->player.room_id];
    needed = game_xp_to_next_level(game->level);
    render_gap();
    RENDER_PRINTF(TXT_HUD_FMT,
        game->tick, room->name, game->player_hp, game->max_hp,
        combat_player_attack_bonus(game),
        game->level, game->xp, needed);
}

void game_print_help(int topic)
{
    RENDER_PRINTF("%s\n", command_help_line(topic));
}

void render_bandit_encounter_open(void)
{
    render_gap();
    RENDER_PRINTF("  /\\     .-'''''''-.        \n");
    RENDER_PRINTF("  ||    / (.)..(.)  |        \n");
    RENDER_PRINTF("  ||    |  (::::)   |        \n");
    RENDER_PRINTF("  ||    \\__________/        \n");
    RENDER_PRINTF(" :::: .-----\\  \\-----.     \n");
    RENDER_PRINTF("  || /                |      \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("%s", TXT_BANDIT_OPEN_INTRO);
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("%s", TXT_BANDIT_OPEN_QUOTE);
    RENDER_PRINTF("%s", TXT_BANDIT_OPEN_OPT1);
    RENDER_PRINTF("%s", TXT_BANDIT_OPEN_OPT2);
    RENDER_PRINTF("%s", TXT_BANDIT_OPEN_OPT3);
    RENDER_PRINTF("%s", TXT_REPLY_PROMPT);
}

void render_combat_start(int player_hp, int enemy_hp)
{
    RENDER_PRINTF(TXT_COMBAT_START_FMT, player_hp, enemy_hp);
    RENDER_PRINTF("%s", TXT_COMBAT_MENU);
}

void render_combat_enemy_strike(int dmg)
{
    RENDER_PRINTF(TXT_COMBAT_ENEMY_STRIKE_FMT, dmg);
}

void render_combat_player_fallen(void)
{
    RENDER_PRINTF("%s", TXT_COMBAT_PLAYER_FALLEN);
}

void render_combat_status_line(int player_hp, int enemy_hp)
{
    RENDER_PRINTF(TXT_COMBAT_STATUS_FMT, player_hp, enemy_hp);
}

void render_combat_player_hit(int dmg)
{
    RENDER_PRINTF(TXT_COMBAT_PLAYER_HIT_FMT, dmg);
}

void render_combat_braced(void)
{
    RENDER_PRINTF("%s", TXT_COMBAT_BRACED);
}

void render_combat_no_salve_bag(void)
{
    RENDER_PRINTF("%s", TXT_COMBAT_NO_SALVE);
}

void render_combat_salve_in_combat(int hp)
{
    RENDER_PRINTF(TXT_COMBAT_SALVE_FMT, hp);
}

void render_combat_salve_full(void)
{
    RENDER_PRINTF("%s", TXT_COMBAT_SALVE_FULL);
    render_already_full_health();
}

void render_already_full_health(void)
{
    RENDER_PRINTF("%s", TXT_ALREADY_FULL_HEALTH);
}

void render_combat_invalid_choice(void)
{
    RENDER_PRINTF("%s", TXT_PICK_123);
}

void render_combat_bandit_defeated(void)
{
    RENDER_PRINTF("%s", TXT_COMBAT_BANDIT_DEFEATED);
}

void render_combat_menu(void)
{
    RENDER_PRINTF("%s", TXT_COMBAT_MENU);
}

void render_xp_gained(int amount)
{
    RENDER_PRINTF(TXT_XP_GAIN_FMT, amount);
}

void render_level_up(int level, int max_hp, int damage_bonus, int bag_capacity)
{
    RENDER_PRINTF(TXT_LEVEL_UP_FMT, level);
    RENDER_PRINTF(TXT_LEVEL_STATS_FMT,
        max_hp, damage_bonus, bag_capacity);
}

void render_nearby_item_notice(const char *item_name)
{
    RENDER_PRINTF(TXT_NEARBY_ITEM_FMT, item_name);
}

void render_animal_noise_line(const char *line)
{
    render_gap();
    RENDER_PRINTF("%s\n", line);
}

void render_atmosphere_gust(void)
{
    render_paragraph(TXT_ATMO_GUST);
}

void render_atmosphere_rustle(void)
{
    render_paragraph(TXT_ATMO_RUSTLE);
}

void render_atmosphere_berry_drop(void)
{
    render_copy(TXT_ATMO_BERRY_DROP);
}

void render_atmosphere_creak(void)
{
    render_paragraph(TXT_ATMO_CREAK);
}

void render_atmosphere_water(void)
{
    render_paragraph(TXT_ATMO_WATER);
}

void render_atmosphere_reed_drop(void)
{
    render_copy(TXT_ATMO_REED_DROP);
}

void render_atmosphere_grit(void)
{
    render_paragraph(TXT_ATMO_GRIT);
}

void render_wanderer_scene(void)
{
    render_gap();
    art_wanderer();
    render_copy(TXT_WANDERER_INTRO);
    RENDER_PRINTF("%s", TXT_WANDERER_QUOTE_A);
    RENDER_PRINTF("%s", TXT_WANDERER_QUOTE_B);
    RENDER_PRINTF("%s", TXT_WANDERER_OPT1);
    RENDER_PRINTF("%s", TXT_WANDERER_OPT2);
    RENDER_PRINTF("%s", TXT_WANDERER_OPT3);
    RENDER_PRINTF("%s", TXT_REPLY_PROMPT);
}

void render_wanderer_reply(int choice)
{
    render_paragraph(txtres_wanderer_reply(choice));
}

void render_frog_dialogue_intro(void)
{
    render_gap();
    art_frog_portrait();
    render_copy(TXT_FROG_INTRO);
    RENDER_PRINTF("%s", TXT_FROG_QUOTE);
    RENDER_PRINTF("%s", TXT_FROG_OPT1);
    RENDER_PRINTF("%s", TXT_FROG_OPT2);
    RENDER_PRINTF("%s", TXT_FROG_OPT3);
    RENDER_PRINTF("%s", TXT_REPLY_PROMPT);
}

void render_frog_dialogue_branch(int choice)
{
    if (choice == 1) {
        render_paragraph(TXT_FROG_REPLY_A1);
        render_copy(TXT_FROG_REPLY_A2);
        render_copy(TXT_FROG_REPLY_A3);
        return;
    }
    if (choice == 2) {
        render_paragraph(TXT_FROG_REPLY_B1);
        render_copy(TXT_FROG_REPLY_B2);
        render_copy(TXT_FROG_REPLY_B3);
        return;
    }
    render_paragraph(TXT_FROG_REPLY_C1);
    render_copy(TXT_FROG_REPLY_C2);
    render_copy(TXT_FROG_REPLY_C3);
}

void render_msg_bandit_waiting_reply(void)
{
    RENDER_PRINTF("%s", TXT_MSG_BANDIT_WAITING);
}

void render_msg_bandit_waiting_handover_pick(void)
{
    RENDER_PRINTF("%s", TXT_MSG_BANDIT_WAITING_HANDOVER);
}

void render_bandit_handover_pick_prompt(void)
{
    RENDER_PRINTF("%s", TXT_BANDIT_HANDOVER_PICK_PROMPT);
}

void render_msg_bandit_give_not_carrying(void)
{
    RENDER_PRINTF("%s", TXT_MSG_BANDIT_GIVE_NOT_CARRYING);
}

void render_msg_give_wrong_context(void)
{
    RENDER_PRINTF("%s", TXT_MSG_GIVE_WRONG_CONTEXT);
}

void render_msg_unknown_command(void)
{
    RENDER_PRINTF("%s", TXT_MSG_UNKNOWN_COMMAND);
}

void render_msg_wait(void)
{
    RENDER_PRINTF("%s", TXT_MSG_WAIT);
}

void render_msg_cannot_move(const char *dir_name)
{
    RENDER_PRINTF(TXT_MSG_CANNOT_MOVE_FMT, dir_name);
}

void render_msg_moved(const char *dir_name)
{
    RENDER_PRINTF(TXT_MSG_MOVED_FMT, dir_name);
}

void render_msg_inspect_nothing(void)
{
    RENDER_PRINTF("%s", TXT_MSG_INSPECT_NOTHING);
}

void render_msg_inspect_wrong_focus(void)
{
    RENDER_PRINTF("%s", TXT_MSG_INSPECT_WRONG_FOCUS);
}

void render_msg_inspect_rustle(void)
{
    RENDER_PRINTF("%s", TXT_MSG_INSPECT_RUSTLE);
}

void render_msg_inspect_creak(void)
{
    RENDER_PRINTF("%s", TXT_MSG_INSPECT_CREAK);
}

void render_msg_inspect_water(void)
{
    RENDER_PRINTF("%s", TXT_MSG_INSPECT_WATER);
}

void render_msg_inspect_grit(void)
{
    RENDER_PRINTF("%s", TXT_MSG_INSPECT_GRIT);
}

void render_msg_bandit_blocks_talk(void)
{
    RENDER_PRINTF("%s", TXT_MSG_BANDIT_BLOCK_TALK);
}

void render_msg_traveler_waiting(void)
{
    RENDER_PRINTF("%s", TXT_MSG_TRAVELER_WAITING);
}

void render_msg_watchman_talk(void)
{
    render_gap();
    art_watchman_portrait();
    render_gap();
    render_copy(TXT_MSG_WATCHMAN_TALK_LINE1);
    render_copy(TXT_MSG_WATCHMAN_TALK_LINE2);
    render_copy(TXT_MSG_WATCHMAN_TALK_LINE3);
    render_copy(TXT_MSG_WATCHMAN_TALK_LINE4);
    render_copy(TXT_REPLY_PROMPT);
}

void render_msg_herbalist_talk(void)
{
    render_gap();
    art_herbalist_portrait();
    render_gap();
    render_copy(TXT_MSG_HERBALIST_TALK_LINE1);
    render_copy(TXT_MSG_HERBALIST_TALK_LINE2);
    render_copy(TXT_MSG_HERBALIST_TALK_LINE3);
    render_copy(TXT_MSG_HERBALIST_TALK_LINE4);
    render_copy(TXT_REPLY_PROMPT);
}

void render_msg_archivist_talk(void)
{
    render_gap();
    art_archivist_portrait();
    render_gap();
    render_copy(TXT_MSG_ARCHIVIST_TALK_LINE1);
    render_copy(TXT_MSG_ARCHIVIST_TALK_LINE2);
    render_copy(TXT_MSG_ARCHIVIST_TALK_LINE3);
    render_copy(TXT_MSG_ARCHIVIST_TALK_LINE4);
    render_copy(TXT_REPLY_PROMPT);
}

void render_msg_nobody_talk(void)
{
    RENDER_PRINTF("%s", TXT_MSG_NOBODY_TALK);
}

void render_msg_watchman_reply(int arg)
{
    RENDER_PRINTF("%s", txtres_msg_watchman_reply(arg));
}

void render_msg_herbalist_reply(int arg)
{
    RENDER_PRINTF("%s", txtres_msg_herbalist_reply(arg));
}

void render_msg_archivist_reply(int arg)
{
    RENDER_PRINTF("%s", txtres_msg_archivist_reply(arg));
}

void render_msg_hand_over_item(const char *item_name)
{
    RENDER_PRINTF(TXT_MSG_HAND_OVER_ITEM_FMT, item_name);
}

void render_msg_bag_empty_bandit(void)
{
    RENDER_PRINTF("%s", TXT_MSG_BAG_EMPTY_BANDIT);
}

void render_msg_intimidate_success(void)
{
    RENDER_PRINTF("%s", TXT_MSG_INTIMIDATE_SUCCESS);
}

void render_msg_intimidate_fail(void)
{
    RENDER_PRINTF("%s", TXT_MSG_INTIMIDATE_FAIL);
}

void render_msg_pick_123(void)
{
    RENDER_PRINTF("%s", TXT_PICK_123);
}

void render_msg_nobody_waiting_reply(void)
{
    RENDER_PRINTF("%s", TXT_MSG_NOBODY_WAITING);
}

void render_inv_no_body_loot(void)
{
    RENDER_PRINTF("%s", TXT_INV_NO_BODY_LOOT);
}

void render_inv_body_stripped(void)
{
    RENDER_PRINTF("%s", TXT_INV_BODY_STRIPPED);
}

void render_inv_bag_full_drop(void)
{
    RENDER_PRINTF("%s", TXT_INV_BAG_FULL_DROP);
}

void render_inv_loot(const char *item_name)
{
    RENDER_PRINTF(TXT_INV_LOOT_FMT, item_name);
}

void render_inv_no_rummage_combat(void)
{
    RENDER_PRINTF("%s", TXT_INV_NO_RUMMAGE_COMBAT);
}

void render_inv_take_nothing(void)
{
    RENDER_PRINTF("%s", TXT_INV_TAKE_NOTHING);
}

void render_inv_cannot_take_here(void)
{
    RENDER_PRINTF("%s", TXT_INV_CANNOT_TAKE_HERE);
}

void render_inv_bag_full(int capacity)
{
    RENDER_PRINTF(TXT_INV_BAG_FULL_FMT, capacity);
}

void render_inv_pickup(const char *item_name)
{
    RENDER_PRINTF(TXT_INV_PICKUP_FMT, item_name);
}

void render_inv_no_drop_combat(void)
{
    RENDER_PRINTF("%s", TXT_INV_NO_DROP_COMBAT);
}

void render_inv_not_carrying(const char *item_name)
{
    RENDER_PRINTF(TXT_INV_NOT_CARRYING_FMT, item_name);
}

void render_inv_ground_full(int slots)
{
    RENDER_PRINTF(TXT_INV_GROUND_FULL_FMT, slots);
}

void render_inv_drop(const char *item_name)
{
    RENDER_PRINTF(TXT_INV_DROP_FMT, item_name);
}

void render_inv_bag(const struct GameState *game)
{
    char list[CFG_FMT_BAG_LIST_MAX];
    int len;

    RENDER_PRINTF(TXT_INV_BAG_HEADER_FMT, game->bag_count, game->bag_capacity);
    if (game->bag_count <= 0) {
        RENDER_PRINTF("%s", TXT_INV_BAG_EMPTY);
    } else {
        len = fmt_inv_bag_items(game, list, (int)sizeof(list));
        if (len >= 0) {
            RENDER_PRINTF("%s\n", list);
        } else {
            RENDER_PRINTF("%s", TXT_INV_BAG_LIST_TOO_LONG);
        }
    }
    if (game->weapon_equipped != ITEM_NONE) {
        RENDER_PRINTF(TXT_INV_BAG_WIELDING_FMT, item_name(game->weapon_equipped));
    }
}

void render_inv_no_eat_combat(void)
{
    RENDER_PRINTF("%s", TXT_INV_NO_EAT_COMBAT);
}

void render_inv_cannot_eat(const char *item_name)
{
    RENDER_PRINTF(TXT_INV_CANNOT_EAT_FMT, item_name);
}

void render_inv_eat_berry_healed(int hp)
{
    RENDER_PRINTF(TXT_INV_EAT_BERRY_HEAL_FMT, hp);
}

void render_inv_eat_berry_full(void)
{
    RENDER_PRINTF("%s", TXT_INV_EAT_BERRY);
    render_already_full_health();
}

void render_inv_eat_fish_healed(int hp)
{
    RENDER_PRINTF(TXT_INV_EAT_FISH_HEAL_FMT, hp);
}

void render_inv_eat_fish_full(void)
{
    RENDER_PRINTF("%s", TXT_INV_EAT_FISH);
    render_already_full_health();
}

void render_inv_use_reply_combat(void)
{
    RENDER_PRINTF("%s", TXT_INV_USE_REPLY_COMBAT);
}

void render_inv_use_torch(void)
{
    RENDER_PRINTF("%s", TXT_INV_USE_TORCH);
}

void render_inv_use_salve(int hp)
{
    RENDER_PRINTF(TXT_INV_USE_SALVE_FMT, hp);
}

void render_inv_use_salve_full(void)
{
    RENDER_PRINTF("%s", TXT_INV_USE_SALVE_FULL);
    render_already_full_health();
}

void render_inv_use_spear(void)
{
    RENDER_PRINTF("%s", TXT_INV_USE_SPEAR);
}

void render_inv_no_use(const char *item_name)
{
    RENDER_PRINTF(TXT_INV_NO_USE_FMT, item_name);
}

void render_inv_no_craft_combat(void)
{
    RENDER_PRINTF("%s", TXT_INV_NO_CRAFT_COMBAT);
}

void render_inv_need_torch(void)
{
    RENDER_PRINTF("%s", TXT_INV_NEED_TORCH);
}

void render_inv_craft_torch(void)
{
    RENDER_PRINTF("%s", TXT_INV_CRAFT_TORCH);
}

void render_inv_need_salve(void)
{
    RENDER_PRINTF("%s", TXT_INV_NEED_SALVE);
}

void render_inv_craft_salve(void)
{
    RENDER_PRINTF("%s", TXT_INV_CRAFT_SALVE);
}

void render_inv_need_spear(void)
{
    RENDER_PRINTF("%s", TXT_INV_NEED_SPEAR);
}

void render_inv_craft_spear(void)
{
    RENDER_PRINTF("%s", TXT_INV_CRAFT_SPEAR);
}

void render_inv_craft_unknown(void)
{
    RENDER_PRINTF("%s", TXT_INV_CRAFT_UNKNOWN);
}

void render_inv_already_wielding(const char *item_name)
{
    RENDER_PRINTF(TXT_INV_ALREADY_WIELDING_FMT, item_name);
}

void render_inv_wield_not_weapon(void)
{
    RENDER_PRINTF("%s", TXT_INV_WIELD_NOT_WEAPON);
}

void render_inv_wield_stow_fail(void)
{
    RENDER_PRINTF("%s", TXT_INV_WIELD_STOW_FAIL);
}

void render_inv_wield(const char *item_name)
{
    RENDER_PRINTF(TXT_INV_WIELD_FMT, item_name);
}

void render_inv_unwield_empty(void)
{
    RENDER_PRINTF("%s", TXT_INV_UNWIELD_EMPTY);
}

void render_inv_unwield(void)
{
    RENDER_PRINTF("%s", TXT_INV_UNWIELD);
}

void render_inv_unwield_cannot(void)
{
    RENDER_PRINTF("%s", TXT_INV_UNWIELD_CANNOT);
}

void render_inv_unwield_ground(const char *item_name)
{
    RENDER_PRINTF(TXT_INV_UNWIELD_GROUND_FMT, item_name);
}

void render_exploration_map(struct GameState *game)
{
    char mapbuf[CFG_FMT_MAP_MAX];
    int len;

    len = fmt_exploration_map(game, mapbuf, (int)sizeof(mapbuf));
    if (len >= 0) {
        RENDER_PRINTF("%s", mapbuf);
    } else {
        RENDER_PRINTF("%s", TXT_MAP_TOO_LARGE);
    }
}
