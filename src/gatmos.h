/*
 * Ambient world feel and room item seeding. The shorter basename keeps the
 * DOS/OpenWatcom tree compatible with FAT 8+3 limits.
 */

#ifndef GATMOS_H
#define GATMOS_H

struct GameState;

void seed_world_items(struct GameState *game);
void maybe_emit_animal_noise(struct GameState *game);
void maybe_emit_atmosphere(struct GameState *game);
int gatmos_cmd_inspect(struct GameState *game, int item_arg);

#endif
