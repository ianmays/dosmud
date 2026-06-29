#include <stdio.h>
#include <stdarg.h>

/*
 * grendr owns terminal presentation: it consumes GameEvent records from core
 * and maps them to printf output, spacing tiers, and ASCII art.
 */

#include "grendr.h"
#include "dialogue.h"
#include "game.h"
#include "npc.h"
#include "gout.h"
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
 * grendr is the only gameplay-adjacent module that prints: it wraps fmt/text
 * resources, adds spacing tiers, and keeps ASCII art out of core gameplay.
 */

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

static void art_traveler(void)
{
    RENDER_PRINTF("        .-''''-.             \n");
    RENDER_PRINTF("       / o)o)   |            \n");
    RENDER_PRINTF("      |   /_    B ------     \n");
    RENDER_PRINTF("      |   ---  / //     \\   \n");
    RENDER_PRINTF("       \\______/_//       \\ \n");
    RENDER_PRINTF("      /        //\\_______/  \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF(" %s", TXT_TRAVELER_ART_CAPTION);
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

static void render_room_look_snapshot(const struct GameState *game, int room_id,
                                      const int *room_items, int corpse_present,
                                      int npc_in_room_hint, int focus_active,
                                      int focus_kind)
{
    char ground_buf[CFG_FMT_GROUND_MAX];
    const struct Room *room;
    int dir;
    int ground_len;

    room = &game->world.rooms[room_id];
    game_print_location_art(room_id);
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
    ground_len = fmt_room_ground_items(room_items, ground_buf,
        (int)sizeof(ground_buf));
    if (ground_len > 0) {
        RENDER_PRINTF("%s", ground_buf);
    } else if (ground_len < 0) {
        RENDER_PRINTF("%s", TXT_UI_GROUND_ITEMS_TOO_LONG);
    }
    if (corpse_present) {
        RENDER_PRINTF("%s", TXT_UI_BANDIT_CORPSE);
    }
    if (npc_in_room_hint != 0) {
        RENDER_PRINTF("%s", TXT_UI_NPC_HINT);
    }
    if (focus_active) {
        if (focus_kind == GAME_ENV_RUSTLE) {
            RENDER_PRINTF("%s", TXT_UI_FOCUS_RUSTLE);
        } else if (focus_kind == GAME_ENV_CREAK) {
            RENDER_PRINTF("%s", TXT_UI_FOCUS_CREAK);
        } else if (focus_kind == GAME_ENV_WATER) {
            RENDER_PRINTF("%s", TXT_UI_FOCUS_WATER);
        } else if (focus_kind == GAME_ENV_GRIT) {
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

void render_msg_version(const char *line)
{
    render_gap();
    RENDER_PRINTF("%s\n", line);
}

/*
 * #158: invent emits GAME_EVENT_ITEM/CRAFT/EQUIP_RESULT and BAG_VIEW; adapters
 * map arg slots to render_inv_* helpers from payload enums.
 */
static void render_item_result_event(const GameEvent *ev)
{
    switch (ev->arg0) {
    case GAME_ITEM_ACTION_LOOT:
        switch (ev->arg1) {
        case GAME_ITEM_OUTCOME_OK:
            render_inv_loot(item_name(ev->arg2));
            break;
        case GAME_ITEM_OUTCOME_NO_BODY:
            render_inv_no_body_loot();
            break;
        case GAME_ITEM_OUTCOME_BODY_STRIPPED:
            render_inv_body_stripped();
            break;
        case GAME_ITEM_OUTCOME_BAG_FULL_DROP:
            render_inv_bag_full_drop();
            break;
        case GAME_ITEM_OUTCOME_LEFT_BEHIND:
            render_inv_leave_body();
            break;
        default:
            break;
        }
        break;
    case GAME_ITEM_ACTION_TAKE:
        switch (ev->arg1) {
        case GAME_ITEM_OUTCOME_OK:
            render_inv_pickup(item_name(ev->arg2));
            break;
        case GAME_ITEM_OUTCOME_BLOCKED_COMBAT:
            render_inv_no_rummage_combat();
            break;
        case GAME_ITEM_OUTCOME_NOTHING_HERE:
            render_inv_take_nothing();
            break;
        case GAME_ITEM_OUTCOME_NOT_HERE:
            render_inv_cannot_take_here();
            break;
        case GAME_ITEM_OUTCOME_BAG_FULL:
            render_inv_bag_full(ev->arg3);
            break;
        default:
            break;
        }
        break;
    case GAME_ITEM_ACTION_DROP:
        switch (ev->arg1) {
        case GAME_ITEM_OUTCOME_OK:
            render_inv_drop(item_name(ev->arg2));
            break;
        case GAME_ITEM_OUTCOME_BLOCKED_COMBAT:
            render_inv_no_drop_combat();
            break;
        case GAME_ITEM_OUTCOME_NOT_CARRYING:
            render_inv_not_carrying(item_name(ev->arg2));
            break;
        case GAME_ITEM_OUTCOME_GROUND_FULL:
            render_inv_ground_full(ev->arg3);
            break;
        default:
            break;
        }
        break;
    case GAME_ITEM_ACTION_EAT:
        switch (ev->arg1) {
        case GAME_ITEM_OUTCOME_OK:
            if (ev->arg2 == ITEM_BERRY) {
                render_inv_eat_berry_healed(ev->arg3);
            } else {
                render_inv_eat_fish_healed(ev->arg3);
            }
            break;
        case GAME_ITEM_OUTCOME_BLOCKED_COMBAT:
            render_inv_no_eat_combat();
            break;
        case GAME_ITEM_OUTCOME_NOT_CARRYING:
            render_inv_not_carrying(item_name(ev->arg2));
            break;
        case GAME_ITEM_OUTCOME_WRONG_ITEM:
            render_inv_cannot_eat(item_name(ev->arg2));
            break;
        case GAME_ITEM_OUTCOME_HP_FULL:
            if (ev->arg2 == ITEM_BERRY) {
                render_inv_eat_berry_full();
            } else {
                render_inv_eat_fish_full();
            }
            break;
        default:
            break;
        }
        break;
    case GAME_ITEM_ACTION_USE:
        switch (ev->arg1) {
        case GAME_ITEM_OUTCOME_OK:
            if (ev->arg2 == ITEM_TORCH) {
                render_inv_use_torch();
            } else if (ev->arg2 == ITEM_SALVE) {
                render_inv_use_salve(ev->arg3);
            } else if (ev->arg2 == ITEM_SPEAR) {
                render_inv_use_spear();
            }
            break;
        case GAME_ITEM_OUTCOME_BLOCKED_COMBAT:
            render_inv_use_reply_combat();
            break;
        case GAME_ITEM_OUTCOME_NOT_CARRYING:
            render_inv_not_carrying(item_name(ev->arg2));
            break;
        case GAME_ITEM_OUTCOME_WRONG_ITEM:
            render_inv_no_use(item_name(ev->arg2));
            break;
        case GAME_ITEM_OUTCOME_HP_FULL:
            render_inv_use_salve_full();
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

static void render_craft_result_event(const GameEvent *ev)
{
    switch (ev->arg1) {
    case GAME_CRAFT_OUTCOME_OK:
        if (ev->arg0 == ITEM_TORCH) {
            render_inv_craft_torch();
        } else if (ev->arg0 == ITEM_SALVE) {
            render_inv_craft_salve();
        } else if (ev->arg0 == ITEM_SPEAR) {
            render_inv_craft_spear();
        }
        break;
    case GAME_CRAFT_OUTCOME_BLOCKED_COMBAT:
        render_inv_no_craft_combat();
        break;
    case GAME_CRAFT_OUTCOME_NEED_INGREDIENTS:
        if (ev->arg0 == ITEM_TORCH) {
            render_inv_need_torch();
        } else if (ev->arg0 == ITEM_SALVE) {
            render_inv_need_salve();
        } else if (ev->arg0 == ITEM_SPEAR) {
            render_inv_need_spear();
        }
        break;
    case GAME_CRAFT_OUTCOME_UNKNOWN:
        render_inv_craft_unknown();
        break;
    default:
        break;
    }
}

static void render_equip_result_event(const GameEvent *ev)
{
    switch (ev->arg1) {
    case GAME_EQUIP_OUTCOME_ALREADY_WIELDING:
        render_inv_already_wielding(item_name(ev->arg0));
        break;
    case GAME_EQUIP_OUTCOME_NOT_CARRYING:
        render_inv_not_carrying(item_name(ev->arg0));
        break;
    case GAME_EQUIP_OUTCOME_NOT_WEAPON:
        render_inv_wield_not_weapon();
        break;
    case GAME_EQUIP_OUTCOME_STOW_FAIL:
        render_inv_wield_stow_fail();
        break;
    case GAME_EQUIP_OUTCOME_WIELDED:
        render_inv_wield(item_name(ev->arg0));
        break;
    case GAME_EQUIP_OUTCOME_UNWIELD_EMPTY:
        render_inv_unwield_empty();
        break;
    case GAME_EQUIP_OUTCOME_UNWIELD_STOWED:
        render_inv_unwield();
        break;
    case GAME_EQUIP_OUTCOME_UNWIELD_CANNOT:
        render_inv_unwield_cannot();
        break;
    case GAME_EQUIP_OUTCOME_UNWIELD_DROPPED:
        render_inv_unwield_ground(item_name(ev->arg0));
        break;
    default:
        break;
    }
}

/*
 * #160: GAME_EVENT_DIALOGUE adapter; arg0=actor, arg1=phase, arg2=choice.
 * arg3 carries authored scene detail for branches that need more than choice.
 * txtres owns the stable actor/phase -> narrative key lookup; grendr only
 * dispatches the resolved key to the matching portrait/menu or reply helper.
 */
static void render_dialogue_event(const GameEvent *ev)
{
    int key;

    key = txtres_dialogue_narrative_key(ev->arg0, ev->arg1);
    switch (key) {
    case TXTRES_NARRATIVE_TRAVELER_REPLY:
        render_traveler_reply(ev->arg2);
        break;
    case TXTRES_NARRATIVE_FROG_TALK:
        render_frog_dialogue_intro();
        break;
    case TXTRES_NARRATIVE_FROG_REPLY:
        render_frog_dialogue_branch(ev->arg2);
        break;
    case TXTRES_NARRATIVE_WATCHMAN_TALK:
        render_msg_watchman_talk();
        break;
    case TXTRES_NARRATIVE_WATCHMAN_REPLY:
        render_msg_watchman_reply(ev->arg2);
        break;
    case TXTRES_NARRATIVE_HERBALIST_TALK:
        render_msg_herbalist_talk(ev->arg3);
        break;
    case TXTRES_NARRATIVE_HERBALIST_REPLY:
        render_msg_herbalist_reply(ev->arg2, ev->arg3);
        break;
    case TXTRES_NARRATIVE_ARCHIVIST_TALK:
        render_msg_archivist_talk();
        break;
    case TXTRES_NARRATIVE_ARCHIVIST_REPLY:
        render_msg_archivist_reply(ev->arg2);
        break;
    case TXTRES_NARRATIVE_NOBODY_TALK:
        render_msg_nobody_talk();
        break;
    default:
        break;
    }
}

/*
 * #160: GAME_EVENT_ENCOUNTER adapter; arg0=kind, arg1=action, arg2=outcome.
 * txtres owns the stable encounter -> narrative key table so bandit/traveler
 * growth does not reopen event-kind branches in the render path.
 */
static void render_encounter_event(const GameEvent *ev)
{
    int key;

    key = txtres_encounter_narrative_key(ev->arg0, ev->arg1, ev->arg2);
    switch (key) {
    case TXTRES_NARRATIVE_BANDIT_OPEN:
        render_bandit_encounter_open(ev->arg3);
        break;
    case TXTRES_NARRATIVE_TRAVELER_SCENE:
        render_traveler_scene();
        break;
    case TXTRES_NARRATIVE_BANDIT_HANDOVER_PROMPT:
        render_bandit_handover_pick_prompt();
        break;
    case TXTRES_NARRATIVE_BANDIT_BAG_EMPTY:
        render_msg_bag_empty_bandit();
        break;
    case TXTRES_NARRATIVE_BANDIT_GIVE_OK:
        render_msg_hand_over_item(ev->text);
        break;
    case TXTRES_NARRATIVE_BANDIT_GIVE_NOT_CARRYING:
        render_msg_bandit_give_not_carrying();
        break;
    case TXTRES_NARRATIVE_BANDIT_GIVE_WRONG_CONTEXT:
        render_msg_give_wrong_context();
        break;
    case TXTRES_NARRATIVE_BANDIT_INTIMIDATE_SUCCESS:
        render_msg_intimidate_success();
        break;
    case TXTRES_NARRATIVE_BANDIT_INTIMIDATE_FAIL:
        render_msg_intimidate_fail();
        break;
    default:
        break;
    }
}

/*
 * #161: GAME_EVENT_ENVIRONMENT adapter; arg0=GameEventEnvironmentKind.
 * Maps GameEventEnvironmentKind to render_atmosphere_* helpers.
 */
static void render_environment_event(const GameEvent *ev)
{
    switch (ev->arg0) {
    case GAME_ENV_EVENT_GUST:
        render_atmosphere_gust();
        break;
    case GAME_ENV_EVENT_RUSTLE:
        render_atmosphere_rustle();
        break;
    case GAME_ENV_EVENT_BERRY_DROP:
        render_atmosphere_berry_drop();
        break;
    case GAME_ENV_EVENT_CREAK:
        render_atmosphere_creak();
        break;
    case GAME_ENV_EVENT_WATER:
        render_atmosphere_water();
        break;
    case GAME_ENV_EVENT_REED_DROP:
        render_atmosphere_reed_drop();
        break;
    case GAME_ENV_EVENT_GRIT:
        render_atmosphere_grit();
        break;
    default:
        break;
    }
}

/*
 * #161: GAME_EVENT_AMBIENT_NOISE adapter; text=animal noise line.
 */
static void render_ambient_noise_event(const GameEvent *ev)
{
    render_animal_noise_line(ev->text);
}

/*
 * #161: GAME_EVENT_ITEM_PRESENCE adapter; text=item name.
 */
static void render_item_presence_event(const GameEvent *ev)
{
    render_nearby_item_notice(ev->text);
}

/*
 * #161: GAME_EVENT_OBSERVATION adapter; arg0=GameEventObservationOutcome.
 */
static void render_observation_event(const GameEvent *ev)
{
    switch (ev->arg0) {
    case GAME_OBS_OUTCOME_NOTHING:
        render_msg_inspect_nothing();
        break;
    case GAME_OBS_OUTCOME_WRONG_FOCUS:
        render_msg_inspect_wrong_focus();
        break;
    case GAME_OBS_OUTCOME_RUSTLE:
        render_msg_inspect_rustle();
        break;
    case GAME_OBS_OUTCOME_CREAK:
        render_msg_inspect_creak();
        break;
    case GAME_OBS_OUTCOME_WATER:
        render_msg_inspect_water();
        break;
    case GAME_OBS_OUTCOME_GRIT:
        render_msg_inspect_grit();
        break;
    default:
        break;
    }
}

/*
 * #160: GAME_EVENT_DIALOGUE_GUARD adapter; arg0=GameEventDialogueGuardReason.
 */
static void render_dialogue_guard_event(const GameEvent *ev)
{
    switch (ev->arg0) {
    case GAME_DIALOGUE_GUARD_BANDIT_WAITING_REPLY:
        render_msg_bandit_waiting_reply();
        break;
    case GAME_DIALOGUE_GUARD_BANDIT_WAITING_HANDOVER_PICK:
        render_msg_bandit_waiting_handover_pick();
        break;
    case GAME_DIALOGUE_GUARD_BANDIT_BLOCKS_TALK:
        render_msg_bandit_blocks_talk();
        break;
    case GAME_DIALOGUE_GUARD_LOOT_WAITING_REPLY:
        render_msg_loot_waiting();
        break;
    case GAME_DIALOGUE_GUARD_TRAVELER_WAITING:
        render_msg_traveler_waiting();
        break;
    case GAME_DIALOGUE_GUARD_NOBODY_WAITING_REPLY:
        render_msg_nobody_waiting_reply();
        break;
    case GAME_DIALOGUE_GUARD_DIALOGUE_CLOSED:
        RENDER_PRINTF("%s", TXT_MSG_DIALOGUE_CLOSED);
        break;
    case GAME_DIALOGUE_GUARD_PICK_123:
        render_msg_pick_123(ev->arg1);
        break;
    default:
        break;
    }
}

/*
 * #159: GAME_EVENT_COMBAT adapter; arg0=phase, arg1/arg2 match combat.c
 * push_combat_phase (see gout.h). Maps phase payloads to render_combat_* helpers.
 */
static void render_combat_event(const GameEvent *ev)
{
    switch (ev->arg0) {
    case GAME_COMBAT_PHASE_START:
        render_combat_start(ev->arg1, ev->arg2, ev->arg3);
        break;
    case GAME_COMBAT_PHASE_ENEMY_DAMAGE:
        render_combat_enemy_strike(ev->arg1);
        break;
    case GAME_COMBAT_PHASE_PLAYER_DOWN:
        render_combat_player_fallen();
        break;
    case GAME_COMBAT_PHASE_STATUS:
        render_combat_status_line(ev->arg1, ev->arg2, ev->arg3);
        break;
    case GAME_COMBAT_PHASE_PLAYER_DAMAGE:
        render_combat_player_hit(ev->arg1);
        break;
    case GAME_COMBAT_PHASE_BRACED:
        render_combat_braced();
        break;
    case GAME_COMBAT_PHASE_SALVE_NO_BAG:
        render_combat_no_salve_bag();
        break;
    case GAME_COMBAT_PHASE_SALVE_HEAL:
        render_combat_salve_in_combat(ev->arg1);
        break;
    case GAME_COMBAT_PHASE_SALVE_FULL:
        render_combat_salve_full();
        break;
    case GAME_COMBAT_PHASE_INVALID_CHOICE:
        render_combat_invalid_choice();
        break;
    case GAME_COMBAT_PHASE_ENEMY_DEFEATED:
        render_combat_bandit_defeated(ev->arg3);
        break;
    case GAME_COMBAT_PHASE_MENU:
        render_combat_menu();
        break;
    default:
        break;
    }
}

/*
 * Drain the per-step event queue in enqueue order. Core must not print; this is
 * the DOSMUD text adapter for generic GameEvent kinds from the simulation queue.
 */
void game_render_output(const struct GameState *game, const GameEventQueue *out)
{
    int i;
    const GameEvent *ev;

    for (i = 0; i < out->count; ++i) {
        ev = &out->events[i];
        switch (ev->kind) {
        case GAME_EVENT_ROOM_LOOK:
            render_room_look_snapshot(game, ev->room_id, ev->room_item,
                ev->arg1, ev->arg0, ev->arg2, ev->arg3);
            break;
        case GAME_EVENT_MOVE:
            render_msg_moved(ev->text);
            break;
        case GAME_EVENT_MAP:
            render_exploration_map(game);
            break;
        case GAME_EVENT_HELP:
            game_print_help(ev->arg0);
            break;
        case GAME_EVENT_VERSION:
            render_msg_version(ev->text);
            break;
        case GAME_EVENT_WAIT:
            render_msg_wait();
            break;
        case GAME_EVENT_CANNOT_MOVE:
            render_msg_cannot_move(ev->text);
            break;
        case GAME_EVENT_UNKNOWN_COMMAND:
            render_msg_unknown_command();
            break;
        /* #158 inventory: direct dispatch (invent no longer wraps LEGACY). */
        case GAME_EVENT_ITEM_RESULT:
            render_item_result_event(ev);
            break;
        case GAME_EVENT_CORPSE_VIEW:
            render_inv_corpse_menu(ev);
            break;
        case GAME_EVENT_BAG_VIEW:
            render_inv_bag(game);
            break;
        case GAME_EVENT_CRAFT_RESULT:
            render_craft_result_event(ev);
            break;
        case GAME_EVENT_EQUIP_RESULT:
            render_equip_result_event(ev);
            break;
        /* #159 combat/progression: direct dispatch (combat/gprog no longer LEGACY). */
        case GAME_EVENT_COMBAT:
            render_combat_event(ev);
            break;
        case GAME_EVENT_XP_GAIN:
            render_xp_gained(ev->arg0);
            break;
        case GAME_EVENT_STAT_CHANGE:
            render_level_up(ev->arg0, ev->arg1, ev->arg2, ev->arg3);
            break;
        /* #160 dialogue/encounter: direct dispatch (slice no longer LEGACY). */
        case GAME_EVENT_DIALOGUE:
            render_dialogue_event(ev);
            break;
        case GAME_EVENT_ENCOUNTER:
            render_encounter_event(ev);
            break;
        case GAME_EVENT_DIALOGUE_GUARD:
            render_dialogue_guard_event(ev);
            break;
        /* #161 ambient/inspect: direct dispatch (gatmos no longer LEGACY). */
        case GAME_EVENT_ENVIRONMENT:
            render_environment_event(ev);
            break;
        case GAME_EVENT_AMBIENT_NOISE:
            render_ambient_noise_event(ev);
            break;
        case GAME_EVENT_ITEM_PRESENCE:
            render_item_presence_event(ev);
            break;
        case GAME_EVENT_OBSERVATION:
            render_observation_event(ev);
            break;
        default:
            break;
        }
    }
}

void render_bandit_encounter_open(int enemy_level)
{
    /* enemy_level is ENCOUNTER OPEN arg3 from npc_push_encounter_open. */
    render_gap();
    RENDER_PRINTF("  /\\     .-'''''''-.        \n");
    RENDER_PRINTF("  ||    / (.)..(.)  |        \n");
    RENDER_PRINTF("  ||    |  (::::)   |        \n");
    RENDER_PRINTF("  ||    \\__________/        \n");
    RENDER_PRINTF(" :::: .-----\\  \\-----.     \n");
    RENDER_PRINTF("  || /                |      \n");
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("%s", TXT_BANDIT_OPEN_INTRO);
    RENDER_PRINTF(TXT_BANDIT_OPEN_LEVEL_FMT, enemy_level);
    RENDER_PRINTF("                             \n");
    RENDER_PRINTF("%s", TXT_BANDIT_OPEN_QUOTE);
    RENDER_PRINTF("%s", TXT_BANDIT_OPEN_OPT1);
    RENDER_PRINTF("%s", TXT_BANDIT_OPEN_OPT2);
    RENDER_PRINTF("%s", TXT_BANDIT_OPEN_OPT3);
    RENDER_PRINTF("%s", TXT_REPLY_PROMPT);
}

void render_combat_start(int player_hp, int enemy_hp, int enemy_level)
{
    RENDER_PRINTF(TXT_COMBAT_START_FMT, player_hp, enemy_hp, enemy_level);
}

void render_combat_enemy_strike(int dmg)
{
    RENDER_PRINTF(TXT_COMBAT_ENEMY_STRIKE_FMT, dmg);
}

void render_combat_player_fallen(void)
{
    RENDER_PRINTF("%s", TXT_COMBAT_PLAYER_FALLEN);
}

void render_combat_status_line(int player_hp, int enemy_hp, int enemy_level)
{
    RENDER_PRINTF(TXT_COMBAT_STATUS_FMT, player_hp, enemy_hp, enemy_level);
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

void render_combat_bandit_defeated(int enemy_level)
{
    if (enemy_level < 1) {
        enemy_level = 1;
    }
    RENDER_PRINTF(TXT_COMBAT_BANDIT_DEFEATED_FMT, enemy_level);
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

void render_traveler_scene(void)
{
    render_gap();
    art_traveler();
    render_copy(TXT_TRAVELER_INTRO);
    RENDER_PRINTF("%s", TXT_TRAVELER_QUOTE_A);
    RENDER_PRINTF("%s", TXT_TRAVELER_QUOTE_B);
    RENDER_PRINTF("%s", TXT_TRAVELER_OPT1);
    RENDER_PRINTF("%s", TXT_TRAVELER_OPT2);
    RENDER_PRINTF("%s", TXT_TRAVELER_OPT3);
    RENDER_PRINTF("%s", TXT_REPLY_PROMPT);
}

void render_traveler_reply(int choice)
{
    render_paragraph(txtres_traveler_reply(choice));
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

void render_msg_loot_waiting(void)
{
    RENDER_PRINTF("%s", TXT_MSG_LOOT_WAITING);
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

void render_msg_herbalist_talk(int scene)
{
    render_gap();
    art_herbalist_portrait();
    render_gap();
    if (scene == HERBALIST_SCENE_REQUESTED) {
        render_copy(TXT_MSG_HERBALIST_REQ_LINE1);
        render_copy(TXT_MSG_HERBALIST_REQ_LINE2);
        render_copy(TXT_MSG_HERBALIST_REQ_LINE3);
        render_copy(TXT_MSG_HERBALIST_REQ_LINE4);
    } else if (scene == HERBALIST_SCENE_READY) {
        render_copy(TXT_MSG_HERBALIST_READY_LINE1);
        render_copy(TXT_MSG_HERBALIST_READY_LINE2);
        render_copy(TXT_MSG_HERBALIST_READY_LINE3);
        render_copy(TXT_MSG_HERBALIST_READY_LINE4);
    } else if (scene == HERBALIST_SCENE_COMPLETE) {
        render_copy(TXT_MSG_HERBALIST_DONE_LINE1);
        render_copy(TXT_MSG_HERBALIST_DONE_LINE2);
        render_copy(TXT_MSG_HERBALIST_DONE_LINE3);
        render_copy(TXT_MSG_HERBALIST_DONE_LINE4);
    } else {
        render_copy(TXT_MSG_HERBALIST_TALK_LINE1);
        render_copy(TXT_MSG_HERBALIST_TALK_LINE2);
        render_copy(TXT_MSG_HERBALIST_TALK_LINE3);
        render_copy(TXT_MSG_HERBALIST_TALK_LINE4);
    }
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

void render_msg_herbalist_reply(int arg, int scene)
{
    RENDER_PRINTF("%s", txtres_msg_herbalist_reply(arg, scene));
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

void render_msg_pick_123(int max_choice)
{
    if (max_choice != 3 && max_choice > 0) {
        RENDER_PRINTF(TXT_PICK_RANGE_FMT, max_choice);
        return;
    }
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

void render_inv_leave_body(void)
{
    RENDER_PRINTF("%s", TXT_INV_LEAVE_BODY);
}

/* arg0 is non-empty item count; room_item[] is the invent snapshot from push_corpse_view. */
void render_inv_corpse_menu(const GameEvent *ev)
{
    int slot;

    RENDER_PRINTF("%s", TXT_INV_CORPSE_HEADER);
    for (slot = 0; slot < ev->arg0; ++slot) {
        RENDER_PRINTF(TXT_INV_CORPSE_LINE_FMT, slot + 1,
            item_name(ev->room_item[slot]));
    }
    RENDER_PRINTF(TXT_INV_CORPSE_LEAVE_FMT, ev->arg1);
    /* loot all is a typed verb (command.c), not a numbered reply choice */
    RENDER_PRINTF("%s", TXT_INV_CORPSE_ALL);
    RENDER_PRINTF(TXT_REPLY_PROMPT_FMT, ev->arg1);
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

void render_exploration_map(const struct GameState *game)
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
