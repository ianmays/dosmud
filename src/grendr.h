#ifndef GRENDR_H
#define GRENDR_H

#include "gout.h"

/*
 * Render/UI declarations. grendr.* stays FAT 8+3 friendly and is the only
 * gameplay-adjacent code allowed to print to stdout.
 */

struct GameState;

#ifdef TEST_MODE
/* Suppress render printf during unit tests (snapshots leave this off). */
void render_set_suppress(int on);
/*
 * Frame line budget (#207): main resets before a shell render frame, then
 * checks after step drain plus HUD. Count is newlines through render_emit plus
 * shell-owned frame rows counted in main_emit; prompt, startup banner, and bye
 * stay outside the budget. over_budget is count > CFG_SAFE_OUTPUT_MAX_LINES
 * (exactly 25 rows is allowed).
 */
void render_frame_begin(void);
int render_frame_line_count(void);
int render_frame_over_budget(void);
#endif

void game_print_location_art(int room_id);
void render_exploration_map(const struct GameState *game);
/* HUD after step output; leading_gap adds a spacer only when the step was blank. */
void game_render(const struct GameState *game, int leading_gap);
/* Drain per-step GameEventQueue; dispatch generic GameEvent kinds to render_* helpers. */
void game_render_output(const struct GameState *game, const GameEventQueue *out);
void game_print_help(int topic);
void render_msg_version(const char *line);

void render_bandit_encounter_open(int enemy_level);
void render_combat_start(int player_hp, int enemy_hp, int enemy_level);
void render_combat_enemy_strike(int dmg);
void render_combat_player_fallen(void);
void render_combat_status_line(int player_hp, int enemy_hp, int enemy_level);
void render_combat_player_hit(int dmg);
void render_combat_braced(void);
void render_combat_no_salve_bag(void);
void render_combat_salve_in_combat(int hp);
void render_combat_salve_full(void);
void render_already_full_health(void);
void render_combat_invalid_choice(void);
void render_combat_bandit_defeated(int enemy_level);
void render_combat_menu(void);

void render_xp_gained(int amount);
void render_level_up(int level, int max_hp, int damage_bonus, int bag_capacity);
void render_nearby_item_notice(const char *item_name);
void render_animal_noise_line(const char *line);
void render_atmosphere_gust(void);
void render_atmosphere_rustle(void);
void render_atmosphere_berry_drop(void);
void render_atmosphere_creak(void);
void render_atmosphere_water(void);
void render_atmosphere_reed_drop(void);
void render_atmosphere_grit(void);
void render_atmosphere_weather_rain(void);
void render_atmosphere_weather_fog(void);
void render_atmosphere_weather_wind(void);
void render_atmosphere_weather_clear(void);
void render_atmosphere_night_fall(void);
void render_atmosphere_day_break(void);
void render_atmosphere_night_lost(void);

void render_traveler_scene(void);
void render_traveler_reply(int choice);
void render_lost_animal_scene(void);
void render_lost_animal_reply(int choice);
void render_peddler_scene(void);
void render_peddler_reply(int choice);
void render_frog_dialogue_intro(void);
void render_frog_dialogue_branch(int choice);

void render_msg_bandit_waiting_reply(void);
void render_msg_bandit_waiting_handover_pick(void);
void render_bandit_handover_pick_prompt(void);
void render_msg_bandit_give_not_carrying(void);
void render_msg_give_wrong_context(void);
void render_msg_unknown_command(void);
void render_msg_wait(void);
void render_msg_cannot_move(const char *dir_name);
void render_msg_moved(const char *dir_name);
void render_msg_inspect_nothing(void);
void render_msg_inspect_lead_spent(int kind);
void render_msg_inspect_choose_kind(void);
void render_msg_inspect_rustle(void);
void render_msg_inspect_creak(void);
void render_msg_inspect_water(void);
void render_msg_inspect_grit(void);
void render_msg_env_menu(int kind);
void render_msg_env_result(int kind, int choice, int detail);
void render_msg_bandit_blocks_talk(void);
void render_msg_loot_waiting(void);
void render_msg_friendly_dialogue_waiting(void);
void render_msg_watchman_talk(int scene);
void render_msg_herbalist_talk(int scene);
void render_msg_archivist_talk(void);
void render_msg_nobody_talk(void);
void render_msg_watchman_reply(int arg, int scene);
void render_msg_herbalist_reply(int arg, int scene);
void render_msg_archivist_reply(int arg);
void render_msg_hand_over_item(const char *item_name);
void render_msg_bag_empty_bandit(void);
void render_msg_intimidate_success(void);
void render_msg_intimidate_fail(void);
void render_msg_pick_123(int max_choice);
void render_msg_nobody_waiting_reply(void);

void render_inv_no_body_loot(void);
void render_inv_body_stripped(void);
void render_inv_bag_full_drop(void);
void render_inv_leave_body(void);
void render_inv_corpse_menu(const GameEvent *ev);
void render_inv_loot(const char *item_name);
void render_inv_no_rummage_combat(void);
void render_inv_take_nothing(void);
void render_inv_cannot_take_here(void);
void render_inv_bag_full(int capacity);
void render_inv_pickup(const char *item_name);
void render_inv_no_drop_combat(void);
void render_inv_not_carrying(const char *item_name);
void render_inv_ground_full(int slots);
void render_inv_drop(const char *item_name);
void render_inv_bag(const struct GameState *game);
void render_inv_no_eat_combat(void);
void render_inv_cannot_eat(const char *item_name);
void render_inv_eat_berry_healed(int hp);
void render_inv_eat_berry_full(void);
void render_inv_eat_fish_healed(int hp);
void render_inv_eat_fish_full(void);
void render_inv_use_reply_combat(void);
void render_inv_use_torch(void);
void render_inv_use_salve(int hp);
void render_inv_use_salve_full(void);
void render_inv_use_spear(void);
void render_inv_no_use(const char *item_name);
void render_inv_no_craft_combat(void);
void render_inv_need_torch(void);
void render_inv_craft_torch(void);
void render_inv_need_salve(void);
void render_inv_craft_salve(void);
void render_inv_need_spear(void);
void render_inv_craft_spear(void);
void render_inv_craft_unknown(void);
void render_inv_already_wielding(const char *item_name);
void render_inv_wield_not_weapon(void);
void render_inv_wield_stow_fail(void);
void render_inv_wield(const char *item_name);
void render_inv_unwield_empty(void);
void render_inv_unwield(void);
void render_inv_unwield_cannot(void);
void render_inv_unwield_ground(const char *item_name);

#endif /* GRENDR_H */
