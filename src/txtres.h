#ifndef TXTRES_H
#define TXTRES_H

/* Static player-facing copy lives here: room text, help text, and dialog
 * strings stay centralized rather than scattered through gameplay code.
 */

extern const char *const g_room_names[];
extern const char *const g_room_descs[];
extern const char *const g_room_animals[];
extern const char *const g_room_noises[];
extern const char *const g_room_art_captions[];
extern const char *const TXT_ROOM_ANIMAL_FALLBACK;
extern const char *const TXT_ROOM_NOISE_FALLBACK;
const char *txtres_dir_name(int dir);

extern const char *const TXT_MAIN_TEST_MODE;
extern const char *const TXT_MAIN_USAGE;
extern const char *const TXT_MAIN_TITLE;
extern const char *const TXT_MAIN_TITLE_SEED_FMT;
extern const char *const TXT_MAIN_HELP_HINT;
extern const char *const TXT_MAIN_PROMPT;
extern const char *const TXT_MAIN_BYE;

extern const char *const TXT_COMMAND_HELP;
extern const char *const TXT_HELP_TOPIC_UNKNOWN;
extern const char *const TXT_HELP_LOOK;
extern const char *const TXT_HELP_MOVE;
extern const char *const TXT_HELP_WAIT;
extern const char *const TXT_HELP_INSPECT;
extern const char *const TXT_HELP_TAKE;
extern const char *const TXT_HELP_DROP;
extern const char *const TXT_HELP_BAG;
extern const char *const TXT_HELP_EAT;
extern const char *const TXT_HELP_USE;
extern const char *const TXT_HELP_CRAFT;
extern const char *const TXT_HELP_LOOT;
extern const char *const TXT_HELP_TALK;
extern const char *const TXT_HELP_REPLY;
extern const char *const TXT_HELP_GIVE;
extern const char *const TXT_HELP_QUIT;
extern const char *const TXT_HELP_HELP;
extern const char *const TXT_HELP_MAP;
extern const char *const TXT_HELP_WIELD;
extern const char *const TXT_HELP_SAVE;
extern const char *const TXT_HELP_LOAD;
extern const char *const TXT_MAP_HEADER;
extern const char *const TXT_MAP_LEGEND;
extern const char *const TXT_MAP_NONE_EXPLORED;
extern const char *const TXT_MAP_TOO_LARGE;

extern const char *const TXT_UI_EXITS_LABEL;
extern const char *const TXT_UI_GROUND_ITEM_FMT;
extern const char *const TXT_UI_GROUND_ITEMS_HEADER;
extern const char *const TXT_UI_GROUND_ITEM_LINE_FMT;
extern const char *const TXT_UI_GROUND_ITEMS_TOO_LONG;
extern const char *const TXT_UI_BANDIT_CORPSE;
extern const char *const TXT_UI_NPC_HINT;
extern const char *const TXT_UI_FOCUS_RUSTLE;
extern const char *const TXT_UI_FOCUS_CREAK;
extern const char *const TXT_UI_FOCUS_WATER;
extern const char *const TXT_UI_FOCUS_GRIT;

extern const char *const TXT_BANDIT_OPEN_INTRO;
extern const char *const TXT_BANDIT_OPEN_QUOTE;
extern const char *const TXT_BANDIT_OPEN_OPT1;
extern const char *const TXT_BANDIT_OPEN_OPT2;
extern const char *const TXT_BANDIT_OPEN_OPT3;
extern const char *const TXT_REPLY_PROMPT;
extern const char *const TXT_REPLY_PROMPT_FMT;

extern const char *const TXT_COMBAT_START_FMT;
extern const char *const TXT_COMBAT_MENU;
extern const char *const TXT_COMBAT_ENEMY_STRIKE_FMT;
extern const char *const TXT_COMBAT_PLAYER_FALLEN;
extern const char *const TXT_COMBAT_STATUS_FMT;
extern const char *const TXT_COMBAT_PLAYER_HIT_FMT;
extern const char *const TXT_COMBAT_BRACED;
extern const char *const TXT_COMBAT_NO_SALVE;
extern const char *const TXT_COMBAT_SALVE_FMT;
extern const char *const TXT_COMBAT_SALVE_FULL;
extern const char *const TXT_ALREADY_FULL_HEALTH;
extern const char *const TXT_PICK_123;
extern const char *const TXT_PICK_RANGE_FMT;
extern const char *const TXT_COMBAT_BANDIT_DEFEATED;

extern const char *const TXT_XP_GAIN_FMT;
extern const char *const TXT_LEVEL_UP_FMT;
extern const char *const TXT_LEVEL_STATS_FMT;
extern const char *const TXT_HUD_FMT;
extern const char *const TXT_NEARBY_ITEM_FMT;
extern const char *const TXT_ATMO_GUST;
extern const char *const TXT_ATMO_RUSTLE;
extern const char *const TXT_ATMO_BERRY_DROP;
extern const char *const TXT_ATMO_CREAK;
extern const char *const TXT_ATMO_WATER;
extern const char *const TXT_ATMO_REED_DROP;
extern const char *const TXT_ATMO_GRIT;

extern const char *const TXT_TRAVELER_INTRO;
extern const char *const TXT_TRAVELER_ART_CAPTION;
extern const char *const TXT_TRAVELER_QUOTE_A;
extern const char *const TXT_TRAVELER_QUOTE_B;
extern const char *const TXT_TRAVELER_OPT1;
extern const char *const TXT_TRAVELER_OPT2;
extern const char *const TXT_TRAVELER_OPT3;
const char *txtres_traveler_reply(int choice);

extern const char *const TXT_FROG_INTRO;
extern const char *const TXT_FROG_ART_CAPTION;
extern const char *const TXT_FROG_QUOTE;
extern const char *const TXT_FROG_OPT1;
extern const char *const TXT_FROG_OPT2;
extern const char *const TXT_FROG_OPT3;
extern const char *const TXT_FROG_REPLY_A1;
extern const char *const TXT_FROG_REPLY_A2;
extern const char *const TXT_FROG_REPLY_A3;
extern const char *const TXT_FROG_REPLY_B1;
extern const char *const TXT_FROG_REPLY_B2;
extern const char *const TXT_FROG_REPLY_B3;
extern const char *const TXT_FROG_REPLY_C1;
extern const char *const TXT_FROG_REPLY_C2;
extern const char *const TXT_FROG_REPLY_C3;

extern const char *const TXT_WATCHMAN_ART_CAPTION;
extern const char *const TXT_HERBALIST_ART_CAPTION;
extern const char *const TXT_ARCHIVIST_ART_CAPTION;

extern const char *const TXT_MSG_BANDIT_WAITING;
extern const char *const TXT_MSG_BANDIT_WAITING_HANDOVER;
extern const char *const TXT_BANDIT_HANDOVER_PICK_PROMPT;
extern const char *const TXT_MSG_BANDIT_GIVE_NOT_CARRYING;
extern const char *const TXT_MSG_GIVE_WRONG_CONTEXT;
extern const char *const TXT_MSG_UNKNOWN_COMMAND;
extern const char *const TXT_MSG_WAIT;
extern const char *const TXT_MSG_CANNOT_MOVE_FMT;
extern const char *const TXT_MSG_MOVED_FMT;
extern const char *const TXT_MSG_INSPECT_NOTHING;
extern const char *const TXT_MSG_INSPECT_WRONG_FOCUS;
extern const char *const TXT_MSG_INSPECT_RUSTLE;
extern const char *const TXT_MSG_INSPECT_CREAK;
extern const char *const TXT_MSG_INSPECT_WATER;
extern const char *const TXT_MSG_INSPECT_GRIT;
extern const char *const TXT_MSG_BANDIT_BLOCK_TALK;
extern const char *const TXT_MSG_LOOT_WAITING;
extern const char *const TXT_MSG_TRAVELER_WAITING;
extern const char *const TXT_MSG_NOBODY_TALK;
const char *txtres_msg_watchman_reply(int arg);
const char *txtres_msg_herbalist_reply(int arg);
const char *txtres_msg_archivist_reply(int arg);
extern const char *const TXT_MSG_HAND_OVER_ITEM_FMT;
extern const char *const TXT_MSG_BAG_EMPTY_BANDIT;
extern const char *const TXT_MSG_INTIMIDATE_SUCCESS;
extern const char *const TXT_MSG_INTIMIDATE_FAIL;
extern const char *const TXT_MSG_NOBODY_WAITING;

extern const char *const TXT_MSG_WATCHMAN_TALK_LINE1;
extern const char *const TXT_MSG_WATCHMAN_TALK_LINE2;
extern const char *const TXT_MSG_WATCHMAN_TALK_LINE3;
extern const char *const TXT_MSG_WATCHMAN_TALK_LINE4;
extern const char *const TXT_MSG_HERBALIST_TALK_LINE1;
extern const char *const TXT_MSG_HERBALIST_TALK_LINE2;
extern const char *const TXT_MSG_HERBALIST_TALK_LINE3;
extern const char *const TXT_MSG_HERBALIST_TALK_LINE4;
extern const char *const TXT_MSG_ARCHIVIST_TALK_LINE1;
extern const char *const TXT_MSG_ARCHIVIST_TALK_LINE2;
extern const char *const TXT_MSG_ARCHIVIST_TALK_LINE3;
extern const char *const TXT_MSG_ARCHIVIST_TALK_LINE4;

extern const char *const TXT_INV_NO_BODY_LOOT;
extern const char *const TXT_INV_BODY_STRIPPED;
extern const char *const TXT_INV_BAG_FULL_DROP;
extern const char *const TXT_INV_LEAVE_BODY;
extern const char *const TXT_INV_CORPSE_HEADER;
extern const char *const TXT_INV_CORPSE_LINE_FMT;
extern const char *const TXT_INV_CORPSE_LEAVE_FMT;
extern const char *const TXT_INV_LOOT_FMT;
extern const char *const TXT_INV_NO_RUMMAGE_COMBAT;
extern const char *const TXT_INV_TAKE_NOTHING;
extern const char *const TXT_INV_CANNOT_TAKE_HERE;
extern const char *const TXT_INV_BAG_FULL_FMT;
extern const char *const TXT_INV_PICKUP_FMT;
extern const char *const TXT_INV_NO_DROP_COMBAT;
extern const char *const TXT_INV_NOT_CARRYING_FMT;
extern const char *const TXT_INV_GROUND_FULL_FMT;
extern const char *const TXT_INV_DROP_FMT;
extern const char *const TXT_INV_BAG_HEADER_FMT;
extern const char *const TXT_INV_BAG_EMPTY;
extern const char *const TXT_INV_BAG_LIST_TOO_LONG;
extern const char *const TXT_INV_NO_EAT_COMBAT;
extern const char *const TXT_INV_CANNOT_EAT_FMT;
extern const char *const TXT_INV_EAT_BERRY;
extern const char *const TXT_INV_EAT_FISH;
extern const char *const TXT_INV_EAT_BERRY_HEAL_FMT;
extern const char *const TXT_INV_EAT_FISH_HEAL_FMT;
extern const char *const TXT_INV_USE_REPLY_COMBAT;
extern const char *const TXT_INV_USE_TORCH;
extern const char *const TXT_INV_USE_SALVE_FMT;
extern const char *const TXT_INV_USE_SALVE_FULL;
extern const char *const TXT_INV_USE_SPEAR;
extern const char *const TXT_INV_NO_USE_FMT;
extern const char *const TXT_INV_NO_CRAFT_COMBAT;
extern const char *const TXT_INV_NEED_TORCH;
extern const char *const TXT_INV_CRAFT_TORCH;
extern const char *const TXT_INV_NEED_SALVE;
extern const char *const TXT_INV_CRAFT_SALVE;
extern const char *const TXT_INV_NEED_SPEAR;
extern const char *const TXT_INV_CRAFT_SPEAR;
extern const char *const TXT_INV_CRAFT_UNKNOWN;
extern const char *const TXT_INV_BAG_WIELDING_FMT;
extern const char *const TXT_INV_WIELD_NOT_WEAPON;
extern const char *const TXT_INV_WIELD_FMT;
extern const char *const TXT_INV_UNWIELD;
extern const char *const TXT_INV_UNWIELD_EMPTY;
extern const char *const TXT_INV_UNWIELD_GROUND_FMT;
extern const char *const TXT_INV_UNWIELD_CANNOT;
extern const char *const TXT_INV_ALREADY_WIELDING_FMT;
extern const char *const TXT_INV_WIELD_STOW_FAIL;

extern const char *const TXT_SAVE_OK_FMT;
extern const char *const TXT_SAVE_IO_FMT;
extern const char *const TXT_LOAD_OK_FMT;
extern const char *const TXT_LOAD_IO_FMT;
extern const char *const TXT_LOAD_BAD_FORMAT;
extern const char *const TXT_LOAD_BAD_RANGE;

#endif
