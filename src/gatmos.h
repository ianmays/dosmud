/*
 * Ambient world feel and room item seeding. The shorter basename keeps the
 * DOS/OpenWatcom tree compatible with FAT 8+3 limits.
 */

#ifndef GATMOS_H
#define GATMOS_H

struct GameState;
struct GameOutput;

void seed_world_items(struct GameState *game);
void maybe_emit_animal_noise(struct GameState *game, struct GameOutput *out);
void maybe_emit_atmosphere(struct GameState *game, struct GameOutput *out);
int gatmos_cmd_inspect(struct GameState *game, int item_arg, struct GameOutput *out);

#endif
