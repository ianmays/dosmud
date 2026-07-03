/*
 * Ambient world feel and room item seeding. The shorter basename keeps the
 * DOS/OpenWatcom tree compatible with FAT 8+3 limits.
 * #51: owns global weather transitions, atmosphere bias, and fog encounter gate.
 */

#ifndef GATMOS_H
#define GATMOS_H

struct GameState;
struct GameEventQueue;

void seed_world_items(struct GameState *game);
void maybe_emit_animal_noise(struct GameState *game, struct GameEventQueue *out);
void maybe_emit_atmosphere(struct GameState *game, struct GameEventQueue *out);
int gatmos_cmd_inspect(struct GameState *game, int item_arg, struct GameEventQueue *out);
/* Numbered reply while env_interact_active; queues ENV_RESULT or guard events. */
int gatmos_cmd_env_reply(struct GameState *game, int choice,
                         struct GameEventQueue *out);
/* Clear menu and queue ENV_MENU_CLOSED guard; game.c calls before blocked verbs. */
void gatmos_env_dismiss(struct GameState *game, struct GameEventQueue *out);
/* Reset env_interact_* only; no events (mode reset and successful reply paths). */
void gatmos_env_clear_interact(struct GameState *game);
/* After load: re-queue ENV_MENU when save restored an active env interaction. */
void gatmos_queue_restored_menu(struct GameState *game,
                                struct GameEventQueue *out);
/* #51: global weather tick and fog roaming-encounter gate (hash-only rolls). */
void gatmos_weather_tick(struct GameState *game, struct GameEventQueue *out);
int gatmos_weather_blocks_roaming_encounter(struct GameState *game);

#endif
