#include "genc.h"
#include "combat.h"
#include "game.h"
#include "gout.h"
#include "invent.h"
#include "items.h"

/*
 * Enemy encounter handling stays separate from combat so the dialogue
 * branch can gate handover and intimidation.
 * #160: queues GAME_EVENT_ENCOUNTER / DIALOGUE_GUARD; grendr maps to text.
 */

/*
 * #160: typed encounter payloads (layout in gout.h). kind/action/outcome ->
 * ENCOUNTER arg0/1/2; item id -> arg3; text for GIVE/OK item names.
 */
static void push_encounter(GameEventQueue *out, int kind, int action,
                           int outcome, int item_id, const char *text)
{
    game_event_push(out, GAME_EVENT_ENCOUNTER, kind, action, outcome, item_id,
        text);
}

static void push_encounter_open(GameEventQueue *out, int kind)
{
    push_encounter(out, kind, GAME_ENCOUNTER_ACTION_OPEN,
        GAME_ENCOUNTER_OUTCOME_NONE, 0, 0);
}

static void push_dialogue_guard(GameEventQueue *out, int reason)
{
    game_event_push(out, GAME_EVENT_DIALOGUE_GUARD, reason, 0, 0, 0, 0);
}

void enemy_begin_encounter(struct GameState *game, GameEventQueue *out)
{
    if (game_is_busy_dialogue(game)) {
        return;
    }
    push_encounter_open(out, GAME_ENCOUNTER_BANDIT);
    game_set_mode_dialogue(game, DIALOGUE_ENEMY);
}

int genc_cmd_give(struct GameState *game, int item_arg, GameEventQueue *out)
{
    if (game->mode == GAME_MODE_DIALOGUE &&
            game->dialogue == DIALOGUE_ENEMY &&
            game->enemy_handover_pick == 1) {
        if (!game_inv_player_has_item(game, item_arg)) {
            push_encounter(out, GAME_ENCOUNTER_BANDIT, GAME_ENCOUNTER_ACTION_GIVE,
                GAME_ENCOUNTER_OUTCOME_NOT_CARRYING, 0, 0);
            return 1;
        }
        push_encounter(out, GAME_ENCOUNTER_BANDIT, GAME_ENCOUNTER_ACTION_GIVE,
            GAME_ENCOUNTER_OUTCOME_OK, item_arg, item_name(item_arg));
        if (game->weapon_equipped == item_arg) {
            game->weapon_equipped = ITEM_NONE;
        } else {
            game_inv_bag_remove_item(game, item_arg);
        }
        game_set_mode_explore(game);
        return 1;
    }
    push_encounter(out, GAME_ENCOUNTER_BANDIT, GAME_ENCOUNTER_ACTION_GIVE,
        GAME_ENCOUNTER_OUTCOME_WRONG_CONTEXT, 0, 0);
    return 1;
}

int genc_cmd_reply(struct GameState *game, int choice, GameEventQueue *out)
{
    /* Reply choice decides whether the enemy opens combat, asks for tribute, or reacts to intimidation. */
    if (choice == 1) {
        combat_start(game, out);
        return 1;
    }
    if (choice == 2) {
        if (game->bag_count <= 0 && game->weapon_equipped == ITEM_NONE) {
            push_encounter(out, GAME_ENCOUNTER_BANDIT,
                GAME_ENCOUNTER_ACTION_HANDOVER,
                GAME_ENCOUNTER_OUTCOME_BAG_EMPTY, 0, 0);
            combat_start(game, out);
            return 1;
        }
        game->enemy_handover_pick = 1;
        push_encounter(out, GAME_ENCOUNTER_BANDIT,
            GAME_ENCOUNTER_ACTION_HANDOVER_PROMPT,
            GAME_ENCOUNTER_OUTCOME_NONE, 0, 0);
        return 1;
    }
    if (choice == 3) {
        game->enemy_handover_pick = 0;
        if (game_roll_percent(game) < CFG_BANDIT_INTIMIDATE_SUCCESS_BELOW) {
            push_encounter(out, GAME_ENCOUNTER_BANDIT,
                GAME_ENCOUNTER_ACTION_INTIMIDATE,
                GAME_ENCOUNTER_OUTCOME_SUCCESS, 0, 0);
            game_set_mode_explore(game);
        } else {
            push_encounter(out, GAME_ENCOUNTER_BANDIT,
                GAME_ENCOUNTER_ACTION_INTIMIDATE,
                GAME_ENCOUNTER_OUTCOME_FAIL, 0, 0);
            combat_start(game, out);
        }
        return 1;
    }
    push_dialogue_guard(out, GAME_DIALOGUE_GUARD_PICK_123);
    return 1;
}
