#include <stdlib.h>
#include "wanderer.h"
#include "game.h"
#include "gout.h"

/*
 * The wanderer is a separate roaming actor, so its movement and encounter
 * gates stay explicit.
 * #160: queues GAME_EVENT_ENCOUNTER / DIALOGUE / DIALOGUE_GUARD; grendr maps.
 */

/*
 * #160: module-local push helpers (same payload layout as dialogue.c/genc.c;
 * see gout.h). Not shared across slices to keep ownership explicit.
 */
static void push_encounter_open(GameEventQueue *out, int kind)
{
    game_event_push(out, GAME_EVENT_ENCOUNTER, kind,
        GAME_ENCOUNTER_ACTION_OPEN, GAME_ENCOUNTER_OUTCOME_NONE, 0, 0);
}

static void push_dialogue(GameEventQueue *out, int actor, int phase,
                          int choice)
{
    game_event_push(out, GAME_EVENT_DIALOGUE, actor, phase, choice, 0, 0);
}

static void push_dialogue_guard(GameEventQueue *out, int reason)
{
    game_event_push(out, GAME_EVENT_DIALOGUE_GUARD, reason, 0, 0, 0, 0);
}

void wanderer_update_separation(struct GameState *game)
{
    /* Once the player leaves the wanderer room, the re-encounter lock can clear. */
    if (game->player.room_id != game->wanderer_room) {
        game->wanderer_need_separation = 0;
    }
}

void wanderer_step(struct GameState *game)
{
    struct Room *r;
    int dirs[CFG_DIR_MAX];
    int n;
    int i;
    int pick;

    /* Wanderer movement is bounded by the generated graph; invalid room state is a no-op. */
    if (game->world.room_count <= 0) {
        return;
    }
    if (game->wanderer_room < 0 || game->wanderer_room >= game->world.room_count) {
        return;
    }
    r = &game->world.rooms[game->wanderer_room];
    n = 0;
    for (i = 0; i < DIR_NONE; ++i) {
        if (r->exits[i] >= 0) {
            dirs[n] = i;
            ++n;
        }
    }
    if (n <= 0) {
        return;
    }
    pick = rand() % n;
    game->wanderer_room = r->exits[dirs[pick]];
}

void wanderer_begin_encounter(struct GameState *game, GameEventQueue *out)
{
    /* The separation flag prevents the same room from retriggering the encounter immediately. */
    if (game_is_busy_dialogue(game)) {
        return;
    }
    if (game->wanderer_need_separation) {
        return;
    }
    push_encounter_open(out, GAME_ENCOUNTER_WANDERER);
    game_set_mode_dialogue(game, DIALOGUE_WANDERER);
    game->wanderer_need_separation = 1;
}

void wanderer_apply_reply(int choice, GameEventQueue *out)
{
    push_dialogue(out, GAME_DIALOGUE_ACTOR_WANDERER, GAME_DIALOGUE_PHASE_REPLY,
        choice);
}

int wanderer_cmd_reply(struct GameState *game, int choice, GameEventQueue *out)
{
    if (choice < 1 || choice > 3) {
        push_dialogue_guard(out, GAME_DIALOGUE_GUARD_PICK_123);
        return 1;
    }
    wanderer_apply_reply(choice, out);
    game_set_mode_explore(game);
    game->wanderer_active = 0;
    game->wanderer_room = -1;
    /* Return timing is randomized only after the player resolves the dialogue branch. */
    game->wanderer_return_tick = game->tick + CFG_WANDERER_RETURN_DELAY_BASE +
        (rand() % CFG_WANDERER_RETURN_DELAY_SPREAD);
    return 1;
}
