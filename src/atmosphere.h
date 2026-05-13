#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

struct GameState;

void seed_world_items(struct GameState *game);
void maybe_emit_animal_noise(struct GameState *game);
void maybe_emit_atmosphere(struct GameState *game);

#endif
