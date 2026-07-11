#include "txtres.h"
#include "config.h"
#include "game.h"
#include "gout.h"
#include "npc.h"
#include "version.h"
#include "world.h"

/*
 * txtres is the single home for static player-facing copy and narrative-scene
 * key tables. Prose and event-to-scene mappings stay here so rendering and
 * gameplay code do not scatter literals or reopen switch ladders.
 */

const char *const g_room_names[CFG_ROOM_MAX] = {
    "Camp", "Road", "Pond", "Forest", "Ruins", "Stream", "Cliff", "Marsh",
    "Grove", "Bridge", "Catacombs", "Meadow", "Canyon", "Tower", "Orchard", "Cave"
};

const char *const g_room_descs[CFG_ROOM_MAX] = {
    "A small campfire burns quietly.",
    "A dusty road stretches toward pale hills.",
    "A still pond reflects the dim sky.",
    "Needles soften your steps; distant branches knit the light.",
    "Broken columns argue with the sky.",
    "Cold water chatters over stones.",
    "Wind gnaws at the cliff edge.",
    "Reeds lean over black water.",
    "Old trees ring a quiet clearing.",
    "A narrow timber bridge bows over the stream.",
    "Cold tunnels run under the ruins.",
    "Tall grass moves in waves around standing stones.",
    "Red stone walls hold old heat and thin echoes.",
    "A watchtower leans above weathered stairs.",
    "Crooked fruit trees crowd a forgotten lane.",
    "A limestone cave breathes cold air from below."
};

const char *const g_room_animals[CFG_ROOM_MAX] = {
    "cricket", "crow", "frog", "owl", "lizard", "otter", "hawk", "heron",
    "fox", "kingfisher", "rat", "hare", "vulture", "raven", "thrush", "bat"
};

const char *const g_room_noises[CFG_ROOM_MAX] = {
    "A cricket chirps from beside the coals.",
    "A crow caws from a leaning signpost.",
    "A frog lets out a smug, wet ribbit.",
    "An owl hoots once, then holds the dark still.",
    "A wall lizard clicks between cracked stones.",
    "An otter splashes and chatters downstream.",
    "A hawk screams high overhead.",
    "A heron rattles through the reeds.",
    "A fox yips from behind the roots.",
    "A kingfisher snaps its beak and dives.",
    "Rats skitter and squeak through the dark.",
    "A hare thumps through the grass nearby.",
    "A vulture croaks from the upper rim.",
    "A raven taps at cracked slate.",
    "A thrush trills from the orchard rows.",
    "Bats chirr overhead in brief bursts."
};

const char *const g_room_art_captions[CFG_ROOM_MAX] = {
    "(campfire at first watch)",
    "(wagon ruts hold rain)",
    "(reeds whisper across water)",
    "(needles dampen every footfall)",
    "(columns eroded by years)",
    "(stones split the current)",
    "wind-scoured ledge",
    "(mud pulls at your boots)",
    "(an old growth ring)",
    "(planks hum underfoot)",
    "(air tastes of chalk)",
    "(seed heads bend in the wind)",
    "(echoes climb the slot)",
    "(watchfire long gone cold)",
    "(fallen fruit scents the air)",
    "(drips mark patient time)"
};

const char *const TXT_ROOM_ANIMAL_FALLBACK = "something";
const char *const TXT_ROOM_NOISE_FALLBACK = "You hear a distant animal noise.";
const char *const TXT_STORY_ORCHARD_DONE_DESC =
    "Crooked fruit trees crowd a forgotten lane, and the bitter scent of crushed marsh-root still hangs in the air.";
const char *const TXT_STORY_TOWER_FED_DESC =
    "A watchtower leans above weathered stairs, and the watchfire burns with a companionable warmth.";

/*
 * actor rows follow GameDialogueActor; columns follow GameDialoguePhase (gout.h).
 * Pad through GAME_DIALOGUE_ACTOR_PEDDLER; bandit slots stay NONE until
 * authored dialogue scenes land.
 */
static const unsigned char g_dialogue_narrative_keys[][3] = {
    { TXTRES_NARRATIVE_NONE, TXTRES_NARRATIVE_NONE, TXTRES_NARRATIVE_NONE },
    { TXTRES_NARRATIVE_NONE, TXTRES_NARRATIVE_FROG_TALK,
        TXTRES_NARRATIVE_FROG_REPLY },
    { TXTRES_NARRATIVE_NONE, TXTRES_NARRATIVE_WATCHMAN_TALK,
        TXTRES_NARRATIVE_WATCHMAN_REPLY },
    { TXTRES_NARRATIVE_NONE, TXTRES_NARRATIVE_HERBALIST_TALK,
        TXTRES_NARRATIVE_HERBALIST_REPLY },
    { TXTRES_NARRATIVE_NONE, TXTRES_NARRATIVE_ARCHIVIST_TALK,
        TXTRES_NARRATIVE_ARCHIVIST_REPLY },
    { TXTRES_NARRATIVE_NONE, TXTRES_NARRATIVE_NONE,
        TXTRES_NARRATIVE_TRAVELER_REPLY },
    { TXTRES_NARRATIVE_NONE, TXTRES_NARRATIVE_NOBODY_TALK,
        TXTRES_NARRATIVE_NONE },
    { TXTRES_NARRATIVE_NONE, TXTRES_NARRATIVE_NONE,
        TXTRES_NARRATIVE_NONE },
    { TXTRES_NARRATIVE_NONE, TXTRES_NARRATIVE_NONE,
        TXTRES_NARRATIVE_NONE },
    { TXTRES_NARRATIVE_NONE, TXTRES_NARRATIVE_NONE,
        TXTRES_NARRATIVE_NONE },
    { TXTRES_NARRATIVE_NONE, TXTRES_NARRATIVE_NONE,
        TXTRES_NARRATIVE_NONE },
    { TXTRES_NARRATIVE_NONE, TXTRES_NARRATIVE_NONE,
        TXTRES_NARRATIVE_LOST_ANIMAL_REPLY },
    { TXTRES_NARRATIVE_NONE, TXTRES_NARRATIVE_NONE,
        TXTRES_NARRATIVE_PEDDLER_REPLY }
};

struct EncounterNarrativeMap {
    unsigned char kind;
    unsigned char action;
    unsigned char outcome;
    unsigned char key;
};

/*
 * Sparse kind/action/outcome rows for GAME_EVENT_ENCOUNTER payloads (gout.h).
 * Roaming-friendly kinds only emit OPEN; bandit owns handover/give/intimidate.
 */
static const struct EncounterNarrativeMap g_encounter_narrative_keys[] = {
    { GAME_ENCOUNTER_BANDIT, GAME_ENCOUNTER_ACTION_OPEN,
        GAME_ENCOUNTER_OUTCOME_NONE, TXTRES_NARRATIVE_BANDIT_OPEN },
    { GAME_ENCOUNTER_TRAVELER, GAME_ENCOUNTER_ACTION_OPEN,
        GAME_ENCOUNTER_OUTCOME_NONE, TXTRES_NARRATIVE_TRAVELER_SCENE },
    { GAME_ENCOUNTER_LOST_ANIMAL, GAME_ENCOUNTER_ACTION_OPEN,
        GAME_ENCOUNTER_OUTCOME_NONE, TXTRES_NARRATIVE_LOST_ANIMAL_SCENE },
    { GAME_ENCOUNTER_PEDDLER, GAME_ENCOUNTER_ACTION_OPEN,
        GAME_ENCOUNTER_OUTCOME_NONE, TXTRES_NARRATIVE_PEDDLER_SCENE },
    { GAME_ENCOUNTER_BANDIT, GAME_ENCOUNTER_ACTION_HANDOVER_PROMPT,
        GAME_ENCOUNTER_OUTCOME_NONE, TXTRES_NARRATIVE_BANDIT_HANDOVER_PROMPT },
    { GAME_ENCOUNTER_BANDIT, GAME_ENCOUNTER_ACTION_HANDOVER,
        GAME_ENCOUNTER_OUTCOME_BAG_EMPTY, TXTRES_NARRATIVE_BANDIT_BAG_EMPTY },
    { GAME_ENCOUNTER_BANDIT, GAME_ENCOUNTER_ACTION_GIVE,
        GAME_ENCOUNTER_OUTCOME_OK, TXTRES_NARRATIVE_BANDIT_GIVE_OK },
    { GAME_ENCOUNTER_BANDIT, GAME_ENCOUNTER_ACTION_GIVE,
        GAME_ENCOUNTER_OUTCOME_NOT_CARRYING,
        TXTRES_NARRATIVE_BANDIT_GIVE_NOT_CARRYING },
    { GAME_ENCOUNTER_BANDIT, GAME_ENCOUNTER_ACTION_GIVE,
        GAME_ENCOUNTER_OUTCOME_WRONG_CONTEXT,
        TXTRES_NARRATIVE_BANDIT_GIVE_WRONG_CONTEXT },
    { GAME_ENCOUNTER_BANDIT, GAME_ENCOUNTER_ACTION_INTIMIDATE,
        GAME_ENCOUNTER_OUTCOME_SUCCESS,
        TXTRES_NARRATIVE_BANDIT_INTIMIDATE_SUCCESS },
    { GAME_ENCOUNTER_BANDIT, GAME_ENCOUNTER_ACTION_INTIMIDATE,
        GAME_ENCOUNTER_OUTCOME_FAIL,
        TXTRES_NARRATIVE_BANDIT_INTIMIDATE_FAIL }
};

const char *txtres_dir_name(int dir)
{
    if (dir == DIR_NORTH) return "north";
    if (dir == DIR_SOUTH) return "south";
    if (dir == DIR_EAST) return "east";
    if (dir == DIR_WEST) return "west";
    return "unknown";
}

int txtres_dialogue_narrative_key(int actor, int phase)
{
    if (actor < 0 ||
            actor >= (int)(sizeof(g_dialogue_narrative_keys) /
                sizeof(g_dialogue_narrative_keys[0]))) {
        return TXTRES_NARRATIVE_NONE;
    }
    if (phase < 0 ||
            phase >= (int)(sizeof(g_dialogue_narrative_keys[0]) /
                sizeof(g_dialogue_narrative_keys[0][0]))) {
        return TXTRES_NARRATIVE_NONE;
    }
    return g_dialogue_narrative_keys[actor][phase];
}

int txtres_encounter_narrative_key(int kind, int action, int outcome)
{
    int i;

    for (i = 0;
            i < (int)(sizeof(g_encounter_narrative_keys) /
                sizeof(g_encounter_narrative_keys[0]));
            ++i) {
        if (g_encounter_narrative_keys[i].kind != kind) {
            continue;
        }
        if (g_encounter_narrative_keys[i].action != action) {
            continue;
        }
        if (g_encounter_narrative_keys[i].outcome != outcome) {
            continue;
        }
        return g_encounter_narrative_keys[i].key;
    }
    return TXTRES_NARRATIVE_NONE;
}

const char *const TXT_MAIN_TEST_MODE = "TEST MODE";
const char *const TXT_MAIN_USAGE =
    "usage: dosmud [--version] [--seed <unsigned>|wallclock] [--replay-log [path]]";
const char *const TXT_MAIN_TITLE = "dosmud";
const char *const TXT_MAIN_TITLE_SEED_FMT = "%s (seed %lu)\n";
const char *const TXT_MAIN_VERSION_FMT = "dosmud " BUILD_VERSION_STRING;
const char *const TXT_MAIN_HELP_HINT = "Type 'help' for commands.";
const char *const TXT_MAIN_PROMPT = "> ";
const char *const TXT_MAIN_BYE = "bye";

const char *const TXT_COMMAND_HELP =
    "Commands: look, map, inspect [rustle|creak|water|grit], take/get/pickup <item> or take all, drop/give/offer <item>, bag, wield <weapon>, unwield, eat/use <item>, craft <item>, loot, move <dir>, wait, talk, 1/2/3 or reply <1-3>, save, load, version, help [topic], quit";

const char *const TXT_HELP_TOPIC_UNKNOWN =
    "No help for that topic. Type 'help' for the full command list.";

const char *const TXT_HELP_LOOK =
    "look (l) - describe the current room, exits, ground items, and prompts.";

const char *const TXT_HELP_MOVE =
    "move <dir> or go <dir>, or n/s/e/w alone - travel when an exit exists.";

const char *const TXT_HELP_WAIT =
    "wait (.) - pass time without moving; the world tick advances.";

const char *const TXT_HELP_INSPECT =
    "inspect <rustle|rustles|creak|creaks|water|grit> - follow a clue named in look. Synonyms: examine, investigate.";

const char *const TXT_HELP_TAKE =
    "take <item> (get, pickup), or take all - pick up items from the ground into your bag.";

const char *const TXT_HELP_DROP =
    "drop <item> - place an item from your bag on the ground here (several items fit until the area is full).";

const char *const TXT_HELP_BAG =
    "bag (inventory, inv) - list what you are carrying.";

const char *const TXT_HELP_EAT =
    "eat <item> - consume edible food from your bag.";

const char *const TXT_HELP_USE =
    "use <item> - use a tool or consumable from your bag (torch, salve, spear).";

const char *const TXT_HELP_CRAFT =
    "craft <item> or build <item> - torch needs stick+reed, salve needs herb+berry, spear needs stick+stone (ingredients in bag or wielded where applicable). Not while fighting.";

const char *const TXT_HELP_LOOT =
    "loot - inspect a bandit corpse, then use the numbered corpse menu to take an item, loot all, or leave the rest.";

const char *const TXT_HELP_TALK =
    "talk (speak, talks) - speak when someone nearby might [talk].";

const char *const TXT_HELP_REPLY =
    "1, 2, 3, or reply <1-3> - answer during bandit dialogue or combat. After choosing 2, use give <item> to surrender one carried item.";

const char *const TXT_HELP_GIVE =
    "give or offer <item> - hand an item to a room NPC when they will accept it, or surrender one item after choosing [2] during a bandit standoff.";

const char *const TXT_HELP_QUIT =
    "quit or exit - leave the game.";

const char *const TXT_HELP_HELP =
    "help or ? lists all commands; help <topic> explains one (e.g. help craft).";

const char *const TXT_HELP_MAP =
    "map (m) - show a small grid of rooms you have visited (compass is approximate).";

const char *const TXT_HELP_WIELD =
    "wield <stick|spear> - ready a carried weapon for combat attacks (bonus damage). unwield - put it away.";

const char *const TXT_HELP_SAVE =
    "save - write the current run to save.dat without advancing time.";

const char *const TXT_HELP_LOAD =
    "load - restore the current run from save.dat without advancing time.";

const char *const TXT_HELP_VERSION =
    "version - print the build identity without advancing time.";

const char *const TXT_MAP_HEADER = "Explored locations:\n";

const char *const TXT_MAP_LEGEND =
    "(@ = you, letter = first initial of a visited place.)\n";

const char *const TXT_MAP_NONE_EXPLORED = "You have not mapped any ground yet.\n";
const char *const TXT_MAP_TOO_LARGE =
    "The explored map is too large to show here.\n";

const char *const TXT_UI_EXITS_LABEL = "Exits:";
const char *const TXT_UI_GROUND_ITEM_FMT = "On the ground: %s. (take %s)\n";
const char *const TXT_UI_GROUND_ITEMS_HEADER = "On the ground:\n";
const char *const TXT_UI_GROUND_ITEM_LINE_FMT = "  %s (take %s)\n";
const char *const TXT_UI_GROUND_ITEMS_TOO_LONG =
    "Too many ground items to list here.\n";
const char *const TXT_UI_BANDIT_CORPSE = "A bandit corpse lies here. (loot)\n";
const char *const TXT_UI_NPC_HINT = "Someone nearby might [talk].\n";
const char *const TXT_LOOK_NIGHT = "Night.";

const char *txtres_look_weather_phrase(int kind)
{
    if (kind == GAME_WEATHER_RAIN) {
        return "Rain needles the air.";
    }
    if (kind == GAME_WEATHER_FOG) {
        return "Fog hangs low and shortens the horizon.";
    }
    if (kind == GAME_WEATHER_WIND) {
        return "Wind keeps worrying the treeline.";
    }
    return 0;
}

const char *txtres_look_clue_phrase(int kind)
{
    if (kind == GAME_ENV_RUSTLE) {
        return "[rustle] nearby";
    }
    if (kind == GAME_ENV_CREAK) {
        return "[creak] nearby";
    }
    if (kind == GAME_ENV_WATER) {
        return "[water] sounds beyond the path";
    }
    if (kind == GAME_ENV_GRIT) {
        return "loose [grit] skittering over stone";
    }
    return 0;
}

const char *const TXT_MAP_NIGHT_BLANK =
    "Darkness and disorientation erase the map from your mind.\n";
const char *const TXT_MAP_TORCH_LIGHT =
    "Your torch illuminates the way ahead.\n";

const char *const TXT_BANDIT_OPEN_INTRO = "A bandit steps from cover with a hand on a rusted blade.\n";
const char *const TXT_BANDIT_OPEN_LEVEL_FMT = "Bandit Lv: %d.\n";
const char *const TXT_BANDIT_OPEN_QUOTE = "\"Easy now. We can do this three ways.\"\n";
const char *const TXT_BANDIT_OPEN_OPT1 = "  [1] Refuse and fight.\n";
const char *const TXT_BANDIT_OPEN_OPT2 =
    "  [2] Hand over one item from your bag or what you are wielding (choose reply 2, then give <item>).\n";
const char *const TXT_BANDIT_OPEN_OPT3 = "  [3] Talk it down and part ways.\n";
const char *const TXT_REPLY_PROMPT = "(Answer with 1, 2, 3, or reply <n>.)\n";
const char *const TXT_REPLY_PROMPT_FMT = "(Answer with 1-%d or reply <n>.)\n";

const char *const TXT_COMBAT_START_FMT =
    "Combat starts. Your HP: %d, Bandit HP: %d, Bandit Lv: %d.\n";
const char *const TXT_COMBAT_MENU = "Choose: [1] Attack  [2] Defend  [3] Use salve\n";
const char *const TXT_COMBAT_ENEMY_STRIKE_FMT = "The bandit strikes for %d damage.\n";
const char *const TXT_COMBAT_PLAYER_FALLEN = "You collapse. The road takes everything.\n";
const char *const TXT_COMBAT_STATUS_FMT =
    "Your HP: %d, Bandit HP: %d, Bandit Lv: %d.\n";
const char *const TXT_COMBAT_PLAYER_HIT_FMT = "You hit the bandit for %d damage.\n";
const char *const TXT_COMBAT_BRACED = "You brace for the incoming strike.\n";
const char *const TXT_COMBAT_NO_SALVE = "You fumble for a salve, but you have none.\n";
const char *const TXT_COMBAT_SALVE_FMT = "You apply salve and recover. HP now %d.\n";
const char *const TXT_COMBAT_SALVE_FULL = "You apply salve.\n";
const char *const TXT_ALREADY_FULL_HEALTH = "You're already at full health.\n";
const char *const TXT_PICK_123 = "Pick 1, 2, or 3.\n";
const char *const TXT_PICK_RANGE_FMT = "Pick 1 through %d.\n";
const char *const TXT_COMBAT_BANDIT_DEFEATED_FMT =
    "The Lv %d bandit falls. The body slumps into the dust.\n";

const char *const TXT_XP_GAIN_FMT = "You gain %d XP.\n";
const char *const TXT_LEVEL_UP_FMT = "Level up! You are now level %d.\n";
const char *const TXT_LEVEL_STATS_FMT = "Max HP %d, Damage bonus +%d, Bag capacity %d.\n";
const char *const TXT_HUD_FMT =
    "[T:%lu] %s [HP:%d/%d] [Atk:%d] [Lv:%d XP:%d/%d]\n";
const char *const TXT_NEARBY_ITEM_FMT = "A %s catches your eye nearby.\n";
const char *const TXT_ATMO_GUST = "A cool gust threads through the area and fades.\n";
const char *const TXT_ATMO_RUSTLE =
    "Something small [rustles] just out of sight.\n";
const char *const TXT_ATMO_BERRY_DROP = "A berry drops from the brush.\n";
const char *const TXT_ATMO_CREAK =
    "A distant [creak] rolls across the landscape.\n";
const char *const TXT_ATMO_WATER =
    "You hear [water] moving somewhere beyond the path.\n";
const char *const TXT_ATMO_REED_DROP = "A loose reed drifts to your feet.\n";
const char *const TXT_ATMO_GRIT =
    "Loose [grit] skips over stone under an uncertain breeze.\n";
const char *const TXT_ATMO_WEATHER_RAIN = "Rain settles in, steady and insistent.\n";
const char *const TXT_ATMO_WEATHER_FOG = "Fog rolls in and muffles distant sound.\n";
const char *const TXT_ATMO_WEATHER_WIND = "Wind rises and worries everything loose.\n";
const char *const TXT_ATMO_WEATHER_CLEAR = "The weather eases and the air clears.\n";
const char *const TXT_ATMO_NIGHT_FALL = "Night falls and the land folds into shadow.\n";
const char *const TXT_ATMO_DAY_BREAK = "Dawn thins the dark and the paths return.\n";
const char *const TXT_ATMO_NIGHT_LOST =
    "Without a torch the night swallows your sense of direction.\n";

const char *const TXT_TRAVELER_INTRO =
    "You nearly bump into a hooded traveler. They straighten with a tired grin.\n";
const char *const TXT_TRAVELER_ART_CAPTION = "(cloak wet with road mist)";
const char *const TXT_TRAVELER_QUOTE_A = "\"Easy there, I'm an adventurer too, working odd jobs between towns. ";
const char *const TXT_TRAVELER_QUOTE_B = "What are you doing out here?\"\n";
const char *const TXT_TRAVELER_OPT1 = "  [1] Looking for trouble worth the trouble.\n";
const char *const TXT_TRAVELER_OPT2 = "  [2] Passing through, keeping my boots honest.\n";
const char *const TXT_TRAVELER_OPT3 = "  [3] That's my business.\n";

const char *txtres_traveler_reply(int choice)
{
    if (choice == 1) {
        return "They nod, amused. \"Bold. Don't trip over your own story.\"\n";
    }
    if (choice == 2) {
        return "They relax a fraction. \"Good. Miles keep liars honest.\"\n";
    }
    return "They raise both hands. \"Fair. The road spies on everyone anyway.\"\n";
}

const char *const TXT_LOST_ANIMAL_INTRO =
    "A shaggy goat trots into view, bells silent and coat dusted with meadow pollen.\n";
const char *const TXT_LOST_ANIMAL_ART_CAPTION = "(lost from some unseen fold)";
const char *const TXT_LOST_ANIMAL_QUOTE_A = "\"Maa,\" it says, then noses your sleeve as if you might know the way home. ";
const char *const TXT_LOST_ANIMAL_QUOTE_B = "What do you do?\"\n";
const char *const TXT_LOST_ANIMAL_OPT1 = "  [1] Shoo it toward the nearest fence line.\n";
const char *const TXT_LOST_ANIMAL_OPT2 = "  [2] Share a handful of trail crumbs.\n";
const char *const TXT_LOST_ANIMAL_OPT3 = "  [3] Leave it to find its own path.\n";

const char *txtres_lost_animal_reply(int choice)
{
    if (choice == 1) {
        return "The goat bleats once, then trots off with offended dignity.\n";
    }
    if (choice == 2) {
        return "It crunches happily, then follows a hoofbeat only it can hear.\n";
    }
    return "It watches you go, then turns back to the tall grass.\n";
}

const char *const TXT_PEDDLER_INTRO =
    "A peddler shifts a creaking pack and catches your eye between the trees.\n";
const char *const TXT_PEDDLER_ART_CAPTION = "(no stall, just stubborn mileage)";
const char *const TXT_PEDDLER_QUOTE_A = "\"Road goods, road prices,\" they say, tapping a bundle of twine. ";
const char *const TXT_PEDDLER_QUOTE_B = "What brings you through the grove?\"\n";
const char *const TXT_PEDDLER_OPT1 = "  [1] Ask what sells on a day like this.\n";
const char *const TXT_PEDDLER_OPT2 = "  [2] Warn them bandits haunt the roads.\n";
const char *const TXT_PEDDLER_OPT3 = "  [3] Keep walking.\n";

const char *txtres_peddler_reply(int choice)
{
    if (choice == 1) {
        return "They grin. \"Patience and string. Folks always need more string.\"\n";
    }
    if (choice == 2) {
        return "Their smile thins. \"Then I keep my coin close and my feet closer.\"\n";
    }
    return "They shrug the pack higher and let the grove swallow the path between you.\n";
}

const char *const TXT_FROG_INTRO =
    "A damp frog wearing an imaginary crown clears his throat.\n";
const char *const TXT_FROG_ART_CAPTION = "(His Majesty, Pond Operations)";
const char *const TXT_FROG_QUOTE = "\"Official pond hours are whenever I say they are. Pick a vibe:\"\n";
const char *const TXT_FROG_OPT1 = "  [1] Bow and wish him a nice pond.\n";
const char *const TXT_FROG_OPT2 = "  [2] Insult his lily pad.\n";
const char *const TXT_FROG_OPT3 = "  [3] Ask if he is a wizard, a snack, or both.\n";
const char *const TXT_FROG_REPLY_A1 = "You bow. The frog salutes with a webbed hand.\n";
const char *const TXT_FROG_REPLY_A2 = "\"Finally, someone whose parents finished the tutorial. ";
const char *const TXT_FROG_REPLY_A3 = "Wisdom of the pond: the water is wet, the mud is judgy, and I am technically management. You're welcome. Ribbit.\"\n";
const char *const TXT_FROG_REPLY_B1 = "You call his lily pad 'discount turf.' ";
const char *const TXT_FROG_REPLY_B2 = "The frog clutches his chest like you stabbed Shakespeare.\n";
const char *const TXT_FROG_REPLY_B3 = "\"Rude! Delicious! That's how you get warts - not magic, just bad networking. Also you're banned from handsomeness.\"\n";
const char *const TXT_FROG_REPLY_C1 =
    "You lean in and whisper that the moon is 'basically a lid.'\n";
const char *const TXT_FROG_REPLY_C2 = "The frog nods with the gravity of a tiny judge.\n";
const char *const TXT_FROG_REPLY_C3 = "\"The moon knows what it did. I'm not allowed to say which phase. If anyone asks, you hallucinated this conversation. For tax reasons.\"\n";

const char *const TXT_WATCHMAN_ART_CAPTION = "(wind-tanned lookout)";
const char *const TXT_HERBALIST_ART_CAPTION = "(hands stained green)";
const char *const TXT_ARCHIVIST_ART_CAPTION = "(ink under every nail)";

const char *const TXT_MSG_BANDIT_WAITING = "The bandit is waiting on your move (reply 1/2/3).\n";
const char *const TXT_MSG_BANDIT_WAITING_HANDOVER =
    "The bandit waits for you to name what you hand over: give <item> (bag lists loose gear; wielded counts).\n";
const char *const TXT_BANDIT_HANDOVER_PICK_PROMPT =
    "The bandit watches your hands. Name what you surrender with give <item>.\n";
const char *const TXT_MSG_BANDIT_GIVE_NOT_CARRYING =
    "You are not carrying that. Pick something from your bag or name your wielded weapon.\n";
const char *const TXT_MSG_GIVE_WRONG_CONTEXT = "Nobody here is asking you for a hand-out.\n";
const char *const TXT_MSG_UNKNOWN_COMMAND = "Unknown command. Type 'help'.\n";
const char *const TXT_MSG_WAIT = "You wait.\n";
const char *const TXT_MSG_CANNOT_MOVE_FMT = "You cannot move %s from here.\n";
const char *const TXT_MSG_MOVED_FMT = "You move %s.\n";
const char *const TXT_MSG_INSPECT_NOTHING = "Nothing here stands out right now.\n";
const char *const TXT_MSG_INSPECT_LEAD_SPENT_FMT =
    "That [%s] lead is spent. Other traces remain. (look)\n";
const char *const TXT_MSG_INSPECT_CHOOSE_KIND =
    "More than one trace stands out. Inspect a [highlighted] word from look.\n";
const char *const TXT_MSG_INSPECT_RUSTLE = "You part the brush and startle a hare into a low sprint.\n";
const char *const TXT_MSG_INSPECT_CREAK = "An old branch rocks against another, groaning like timber.\n";
const char *const TXT_MSG_INSPECT_WATER = "You find a thin runnel cutting fresh lines through mud.\n";
const char *const TXT_MSG_INSPECT_GRIT = "New tracks cross the grit: light, quick, and already fading.\n";
const char *const TXT_MSG_ENV_MENU_WATER =
    "  [1] Follow the runnel downstream.\n  [2] Cup your hands and drink.\n  [3] Step back.\n";
const char *const TXT_MSG_ENV_MENU_RUSTLE =
    "  [1] Search the brush.\n  [2] Step back.\n";
const char *const TXT_MSG_ENV_MENU_CREAK =
    "  [1] Pull free a splinter.\n  [2] Step back.\n";
const char *const TXT_MSG_ENV_MENU_GRIT =
    "  [1] Study the tracks.\n  [2] Step back.\n";
const char *const TXT_MSG_ENV_RESULT_WATER_FOLLOW =
    "The runnel bends toward the stream bed and loses itself in reeds.\n";
const char *const TXT_MSG_ENV_RESULT_WATER_DRINK =
    "You cup the water. It is cold and clears your head.\n";
const char *const TXT_MSG_ENV_RESULT_WATER_DRINK_FULL =
    "You sip, but you already feel steady.\n";
const char *const TXT_MSG_ENV_RESULT_WATER_LEAVE =
    "You step back from the muddy runnel.\n";
const char *const TXT_MSG_ENV_RESULT_RUSTLE_SEARCH =
    "You comb the brush and find a berry glinting below.\n";
const char *const TXT_MSG_ENV_RESULT_RUSTLE_SEARCH_FAIL =
    "The undergrowth is too tangled to turn up more.\n";
const char *const TXT_MSG_ENV_RESULT_RUSTLE_LEAVE =
    "You let the brush settle.\n";
const char *const TXT_MSG_ENV_RESULT_CREAK_SPLINTER =
    "You tug free a dry splinter.\n";
const char *const TXT_MSG_ENV_RESULT_CREAK_SPLINTER_FAIL =
    "The branch holds firm; nothing worth keeping.\n";
const char *const TXT_MSG_ENV_RESULT_CREAK_LEAVE =
    "You leave the creaking timber alone.\n";
const char *const TXT_MSG_ENV_RESULT_GRIT_STUDY =
    "The tracks double back once, then vanish where the grit thins.\n";
const char *const TXT_MSG_ENV_RESULT_GRIT_LEAVE =
    "The grit gives no further answer.\n";
const char *const TXT_MSG_ENV_MENU_CLOSED =
    "You turn away from the distraction.\n";
const char *const TXT_MSG_BANDIT_BLOCK_TALK = "The bandit has your full attention right now.\n";
const char *const TXT_MSG_LOOT_WAITING =
    "Finish looting with the numbered corpse menu, or type loot to leave the body alone.\n";
const char *const TXT_MSG_FRIENDLY_DIALOGUE_WAITING =
    "They are waiting on your move (reply 1/2/3).\n";
const char *const TXT_MSG_ENV_MENU_WAITING =
    "Finish the prompt with its numbered choices, or move to step away.\n";
const char *const TXT_MSG_NOBODY_TALK = "Nobody here wants to talk.\n";
const char *const TXT_MSG_DIALOGUE_CLOSED = "You leave the conversation and turn back to the room.\n";
const char *const TXT_MSG_GIVE_NO_TARGET = "Nobody here is waiting for an offered item.\n";
const char *const TXT_MSG_GIVE_REJECTED = "That offer goes nowhere.\n";
const char *const TXT_MSG_HAND_OVER_ITEM_FMT = "You hand over your %s. The bandit backs off and leaves.\n";
const char *const TXT_MSG_BAG_EMPTY_BANDIT = "Your bag is empty. The bandit laughs and attacks.\n";
const char *const TXT_MSG_INTIMIDATE_SUCCESS = "You keep your voice steady. The bandit grunts and withdraws.\n";
const char *const TXT_MSG_INTIMIDATE_FAIL = "Your pitch fails. The bandit lunges.\n";
const char *const TXT_MSG_NOBODY_WAITING =
    "No numbered choice is open. Use normal commands like look, move, loot, or talk.\n";

const char *const TXT_MSG_WATCHMAN_TALK_LINE1 = "A one-eyed watchman leans on the parapet.\n";
const char *const TXT_MSG_WATCHMAN_TALK_LINE2 = "\"Storms come from the canyon. You carry a torch?\"\n";
const char *const TXT_MSG_WATCHMAN_TALK_LINE3 = "  [1] Ask for warning signs.\n  [2] Offer to share a meal.\n";
const char *const TXT_MSG_WATCHMAN_TALK_LINE3_WARNED =
    "  [1] Ask about warning signs again.\n  [2] Offer to share a meal.\n";
const char *const TXT_MSG_WATCHMAN_TALK_LINE3_FED =
    "  [1] Ask for warning signs.\n  [2] Offer to share a meal again.\n";
const char *const TXT_MSG_WATCHMAN_TALK_LINE3_WARNED_FED =
    "  [1] Ask about warning signs again.\n  [2] Offer to share a meal again.\n";
const char *const TXT_MSG_WATCHMAN_TALK_LINE4 = "  [3] Say nothing and move on.\n";
const char *const TXT_MSG_WATCHMAN_AFTER_WARN_OPTIONS =
    "  [1] Ask what to do when it hits.\n  [2] Change the subject.\n  [3] Leave him to his watch.\n";
const char *const TXT_MSG_WATCHMAN_MEAL_GIVE_PROMPT =
    "Name what you offer with give <item> or offer <item>.\n";
const char *const TXT_MSG_WATCHMAN_MEAL_OPTIONS =
    "  [1] Apologize - no food right now.\n  [2] Leave.\n";
const char *const TXT_MSG_HERBALIST_TALK_LINE1 = "An herbalist kneels among fallen fruit.\n";
const char *const TXT_MSG_HERBALIST_TALK_LINE2 = "\"I could use a steady pair of hands, if you're offering them.\"\n";
const char *const TXT_MSG_HERBALIST_TALK_LINE3 = "  [1] Ask what she needs.\n  [2] Trade gossip from the road.\n";
const char *const TXT_MSG_HERBALIST_TALK_LINE4 = "  [3] Leave politely.\n";
const char *const TXT_MSG_HERBALIST_REQ_LINE1 = "The herbalist brushes pulp from her palms.\n";
const char *const TXT_MSG_HERBALIST_REQ_LINE2 = "\"Bring me a marsh-root from the black water marsh. Freshly cut, not drowned.\"\n";
const char *const TXT_MSG_HERBALIST_REQ_ROOT_LINE3 =
    "  [1] Ask about the marsh-root again.\n  [2] Trade gossip from the road.\n";
const char *const TXT_MSG_HERBALIST_REQ_ROOT_LINE4 = "  [3] Leave politely.\n";
const char *const TXT_MSG_HERBALIST_REQ_LINE3 = "  [1] Ask where to look.\n  [2] Admit you do not have it yet.\n";
const char *const TXT_MSG_HERBALIST_REQ_LINE4 = "  [3] Leave and keep searching.\n";
const char *const TXT_MSG_HERBALIST_READY_LINE1 = "The herbalist spots the marsh-root in your bag at once.\n";
const char *const TXT_MSG_HERBALIST_READY_LINE2 = "\"You found one. Will you hand it over?\"\n";
const char *const TXT_MSG_HERBALIST_READY_LINE3 = "  [1] Give her the marsh-root.\n  [2] Ask why she needs it.\n";
const char *const TXT_MSG_HERBALIST_READY_LINE4 = "  [3] Keep it for now.\n";
const char *const TXT_MSG_HERBALIST_DONE_LINE1 = "The herbalist ties the cut root in drying twine.\n";
const char *const TXT_MSG_HERBALIST_DONE_LINE2 = "\"The worst of the blight should break by morning. What else are you after?\"\n";
const char *const TXT_MSG_HERBALIST_DONE_LINE3 = "  [1] Ask where to search next.\n  [2] Ask what the root cured.\n";
const char *const TXT_MSG_HERBALIST_DONE_LINE4 = "  [3] Leave her to the work.\n";
const char *const TXT_MSG_ARCHIVIST_TALK_LINE1 = "A dust-caked archivist lights a stub candle.\n";
const char *const TXT_MSG_ARCHIVIST_TALK_LINE2 = "\"Speak quickly. Stone remembers everything.\"\n";
const char *const TXT_MSG_ARCHIVIST_TALK_LINE3 = "  [1] Ask about the ruins.\n  [2] Ask about safer routes.\n";
const char *const TXT_MSG_ARCHIVIST_TALK_LINE4 = "  [3] Thank them and leave.\n";

/*
 * scene is WatchmanDialogueScene from npc.c.
 */
const char *txtres_msg_watchman_reply(int arg, int scene)
{
    if (scene == WATCHMAN_SCENE_PECKISH) {
        return "He pauses. \"Actually, now that you mention it, I have been feeling a bit peckish.\"\n";
    }
    if (scene == WATCHMAN_SCENE_WARNING) {
        return "He points west. \"If crows go quiet, squall in ten minutes.\"\n";
    }
    if (scene == WATCHMAN_SCENE_SQUALL_ADVICE) {
        return "He barks over the wind. \"Find low ground and ditch anything iron.\"\n";
    }
    if (scene == WATCHMAN_SCENE_CHANGE_SUBJECT) {
        return "He grunts. \"Say your piece, then.\"\n";
    }
    if (scene == WATCHMAN_SCENE_APOLOGY) {
        return "He nods once. \"Fair enough. Bring something when you can.\"\n";
    }
    if (scene == WATCHMAN_SCENE_FOOD_THANKS) {
        return "He takes the food and chews slowly. \"That hit the spot.\"\n";
    }
    if (scene == WATCHMAN_SCENE_ALREADY_FED) {
        return "He pats his belt. \"I already ate. Save the meal for the road.\"\n";
    }
    if (scene == WATCHMAN_SCENE_GIVE_NOT_CARRYING) {
        return "He checks your hands. \"Name something edible you are actually carrying.\"\n";
    }
    if (scene == WATCHMAN_SCENE_GIVE_REJECTED) {
        return "He is not asking for that right now.\n";
    }
    if (scene == WATCHMAN_SCENE_AFTER_WARNING) {
        return "He turns back to the horizon without another word.\n";
    }
    if (scene == WATCHMAN_SCENE_MEAL_OFFER ||
            scene == WATCHMAN_SCENE_MEAL_OFFER_EMPTY) {
        return "He lets the silence settle and keeps watching the canyon.\n";
    }
    if (scene == WATCHMAN_SCENE_NEUTRAL) {
        if (arg == 2) {
            return "He pauses. \"Actually, now that you mention it, I have been feeling a bit peckish.\"\n";
        }
        return "He nods once and returns to the horizon.\n";
    }
    return "He nods once and returns to the horizon.\n";
}

const char *txtres_msg_herbalist_reply(int arg, int scene)
{
    if (scene == HERBALIST_SCENE_NOT_STARTED) {
        if (arg == 1) {
            return "She lowers her voice. \"The marsh still grows a root that cuts fever fast. Bring me one, and I'll point you toward older answers than mine.\"\n";
        }
        if (arg == 2) {
            return "She smiles thinly. \"Road gossip keeps longer than fruit. Marsh-root does not.\"\n";
        }
        return "She waves without looking up.\n";
    }
    if (scene == HERBALIST_SCENE_REQUESTED) {
        if (arg == 1) {
            return "She gestures with a stained thumb. \"Look where the reeds crowd black water. The root knots low in the mud.\"\n";
        }
        if (arg == 2) {
            return "She nods once. \"Then do not linger here. The marsh keeps what waits too long.\"\n";
        }
        return "She turns back to sorting bruised fruit.\n";
    }
    if (scene == HERBALIST_SCENE_READY) {
        if (arg == 1) {
            return "She studies the root in your hands but waits for you to hand it over properly.\nUse give marsh-root if you want her to take it now.\n";
        }
        if (arg == 2) {
            return "\"A child upriver is burning with marsh fever,\" she says. \"This cuts it before dusk does.\"\n";
        }
        return "She watches the root in your bag and says nothing more.\n";
    }
    if (scene == HERBALIST_SCENE_GIVE_REJECTED) {
        return "She shakes her head. \"Not that. Bring me a marsh-root if you mean to help.\"\n";
    }
    if (scene == HERBALIST_SCENE_GIVE_NOT_CARRYING) {
        return "Her eyes flick to your empty hands. \"Come back when the marsh-root is actually with you.\"\n";
    }
    if (scene == HERBALIST_SCENE_GIVE_REWARD_BAG) {
        return "She takes the marsh-root and crushes it between her stained thumbs.\nThe bitter scent clears the orchard air for a moment.\nShe presses a salve into your bag before turning back to her mortar.\n\"If you are still hunting answers, start with the Ruins. If the stones lie, try the Catacombs beneath them.\"\n";
    }
    if (scene == HERBALIST_SCENE_GIVE_REWARD_GROUND) {
        return "She takes the marsh-root and crushes it between her stained thumbs.\nThe bitter scent clears the orchard air for a moment.\nYour bag is full, so she leaves a salve at your feet before returning to her mortar.\n\"If you are still hunting answers, start with the Ruins. If the stones lie, try the Catacombs beneath them.\"\n";
    }
    if (scene == HERBALIST_SCENE_GIVE_REWARD_NO_SPACE) {
        return "She glances from your full bag to the cluttered ground and does not take the root.\n\"Make room first. I will not waste the cure in the dirt.\"\n";
    }
    if (arg == 1) {
        return "She jerks her chin east. \"The Ruins keep the first layer of truth. The Catacombs keep what sank beneath it.\"\n";
    }
    if (arg == 2) {
        return "She binds the paste into cloth. \"Enough to cool a fever and buy one family another week.\"\n";
    }
    return "She nods her thanks and returns to her mortar.\n";
}

const char *txtres_msg_archivist_reply(int arg)
{
    if (arg == 1) return "Archivist: \"The top stones cracked first. The foundations were already wrong.\"\n";
    if (arg == 2) return "Archivist: \"Follow running water; dead tunnels lie to travelers.\"\n";
    return "Archivist: \"Go, then. Before the candle quits.\"\n";
}

const char *const TXT_INV_NO_BODY_LOOT = "There is no body here to loot.\n";
const char *const TXT_INV_BODY_STRIPPED = "The body has already been stripped clean.\n";
const char *const TXT_INV_BAG_FULL_DROP = "Your bag is full. Drop something first.\n";
const char *const TXT_INV_LEAVE_BODY = "You leave the rest on the body.\n";
const char *const TXT_INV_CORPSE_HEADER = "On the body:\n";
const char *const TXT_INV_CORPSE_LINE_FMT = "  [%d] %s\n";
const char *const TXT_INV_CORPSE_LEAVE_FMT = "  [%d] Leave the rest.\n";
const char *const TXT_INV_CORPSE_ALL = "Type loot all to grab everything you can carry.\n";
const char *const TXT_INV_LOOT_FMT = "You loot a %s from the body.\n";
const char *const TXT_INV_NO_RUMMAGE_COMBAT = "You cannot rummage through gear mid-fight.\n";
const char *const TXT_INV_TAKE_NOTHING = "There is nothing here to take.\n";
const char *const TXT_INV_CANNOT_TAKE_HERE = "You cannot take that from here.\n";
const char *const TXT_INV_BAG_FULL_FMT = "Your bag is full (%d items max).\n";
const char *const TXT_INV_PICKUP_FMT = "You pick up the %s.\n";
const char *const TXT_INV_NO_DROP_COMBAT = "Not while a blade is in your face.\n";
const char *const TXT_INV_NOT_CARRYING_FMT = "You are not carrying a %s.\n";
const char *const TXT_INV_GROUND_FULL_FMT =
    "The ground here cannot hold more than %d items.\n";
const char *const TXT_INV_DROP_FMT = "You drop the %s.\n";
const char *const TXT_INV_BAG_HEADER_FMT = "Bag (%d/%d):";
const char *const TXT_INV_BAG_EMPTY = " empty\n";
const char *const TXT_INV_BAG_LIST_TOO_LONG =
    "Your bag list is too long to show here.\n";
const char *const TXT_INV_NO_EAT_COMBAT = "You cannot eat calmly during combat.\n";
const char *const TXT_INV_CANNOT_EAT_FMT = "You cannot eat the %s.\n";
const char *const TXT_INV_EAT_BERRY = "You eat the berry. Tart, but fresh.\n";
const char *const TXT_INV_EAT_FISH = "You eat the fish. Not ideal raw, but nourishing.\n";
const char *const TXT_INV_EAT_BERRY_HEAL_FMT =
    "You eat the berry. Tart, but fresh. You recover 1 HP. HP now %d.\n";
const char *const TXT_INV_EAT_FISH_HEAL_FMT =
    "You eat the fish. Not ideal raw, but nourishing. You recover 2 HP. HP now %d.\n";
const char *const TXT_INV_USE_REPLY_COMBAT = "In combat, use reply 1/2/3 for your turn.\n";
const char *const TXT_INV_USE_TORCH = "You raise the torch; nearby details sharpen in warm light.\n";
const char *const TXT_INV_USE_SALVE_FMT = "You apply the salve and recover 5 HP. HP now %d.\n";
const char *const TXT_INV_USE_SALVE_FULL = "You apply the salve.\n";
const char *const TXT_INV_USE_SPEAR = "You test the spear's weight. Balanced enough.\n";
const char *const TXT_INV_NO_USE_FMT = "You cannot find a practical use for the %s right now.\n";
const char *const TXT_INV_NO_CRAFT_COMBAT = "You cannot craft while fighting.\n";
const char *const TXT_INV_NEED_TORCH = "Craft torch needs: stick + reed.\n";
const char *const TXT_INV_CRAFT_TORCH = "You bind a makeshift torch.\n";
const char *const TXT_INV_NEED_SALVE = "Craft salve needs: herb + berry.\n";
const char *const TXT_INV_CRAFT_SALVE = "You mash a basic healing salve.\n";
const char *const TXT_INV_NEED_SPEAR = "Craft spear needs: stick + stone.\n";
const char *const TXT_INV_CRAFT_SPEAR = "You lash a stone point to the stick and craft a spear.\n";
const char *const TXT_INV_CRAFT_UNKNOWN = "You do not know how to craft that.\n";
const char *const TXT_INV_BAG_WIELDING_FMT = "Wielding: %s\n";
const char *const TXT_INV_WIELD_NOT_WEAPON = "That is not something you can wield as a weapon.\n";
const char *const TXT_INV_WIELD_FMT = "You ready the %s.\n";
const char *const TXT_INV_UNWIELD = "You sling your weapon aside.\n";
const char *const TXT_INV_UNWIELD_EMPTY = "You have nothing wielded.\n";
const char *const TXT_INV_UNWIELD_GROUND_FMT =
    "Your bag is full; you set the %s on the ground.\n";
const char *const TXT_INV_UNWIELD_CANNOT =
    "You cannot unwield right now. Your bag is full and there is no room on the ground here.\n";
const char *const TXT_INV_ALREADY_WIELDING_FMT = "You are already wielding the %s.\n";
const char *const TXT_INV_WIELD_STOW_FAIL =
    "You cannot switch weapons; there is no room to stow what you were holding.\n";

const char *const TXT_SAVE_OK_FMT = "Saved to %s.\n";
const char *const TXT_SAVE_IO_FMT = "Could not save to %s.\n";
const char *const TXT_LOAD_OK_FMT = "Loaded from %s.\n";
const char *const TXT_LOAD_IO_FMT = "Could not load from %s.\n";
const char *const TXT_LOAD_BAD_FORMAT = "Save file is not a supported dosmud save.\n";
const char *const TXT_LOAD_BAD_RANGE = "Save file contents are out of range.\n";
