#ifndef GRENDR_H
#define GRENDR_H

/* Game render / UI layer. Source files are grendr.* so DOS 8.3 hosts can open them. */
/* Stored in GameState.env_focus_kind; keep in sync with game logic. */
#define GAME_ENV_NONE 0
#define GAME_ENV_RUSTLE 1
#define GAME_ENV_CREAK 2
#define GAME_ENV_WATER 3
#define GAME_ENV_GRIT 4

struct GameState;

void game_print_location_art(int room_id);
void render_room_look(struct GameState *game, int npc_in_room_hint);
void game_render(const struct GameState *game);
void game_print_help(int topic);

void render_bandit_encounter_open(void);
void render_combat_start(int player_hp, int enemy_hp);
void render_combat_enemy_strike(int dmg);
void render_combat_player_fallen(void);
void render_combat_status_line(int player_hp, int enemy_hp);
void render_combat_player_hit(int dmg);
void render_combat_braced(void);
void render_combat_no_salve_bag(void);
void render_combat_salve_in_combat(int hp);
void render_combat_invalid_choice(void);
void render_combat_bandit_defeated(void);
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

void render_wanderer_scene(void);
void render_wanderer_reply(int choice);
void render_frog_dialogue_intro(void);
void render_frog_dialogue_branch(int choice);

void render_msg_bandit_waiting_reply(void);
void render_msg_unknown_command(void);
void render_msg_wait(void);
void render_msg_cannot_move(const char *dir_name);
void render_msg_moved(const char *dir_name);
void render_msg_inspect_nothing(void);
void render_msg_inspect_wrong_focus(void);
void render_msg_inspect_rustle(void);
void render_msg_inspect_creak(void);
void render_msg_inspect_water(void);
void render_msg_inspect_grit(void);
void render_msg_bandit_blocks_talk(void);
void render_msg_traveler_waiting(void);
void render_msg_watchman_talk(void);
void render_msg_herbalist_talk(void);
void render_msg_archivist_talk(void);
void render_msg_nobody_talk(void);
void render_msg_watchman_reply(int arg);
void render_msg_herbalist_reply(int arg);
void render_msg_archivist_reply(int arg);
void render_msg_hand_over_item(const char *item_name);
void render_msg_bag_empty_bandit(void);
void render_msg_intimidate_success(void);
void render_msg_intimidate_fail(void);
void render_msg_pick_123(void);
void render_msg_nobody_waiting_reply(void);

#endif /* GRENDR_H */
