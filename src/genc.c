#include "genc.h"
#include "combat.h"
#include "game.h"
#include "gout.h"
#include "invent.h"
#include "items.h"
#include "npc.h"

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

static void push_dialogue_guard(GameEventQueue *out, int reason)
{
    game_event_push(out, GAME_EVENT_DIALOGUE_GUARD, reason, 0, 0, 0, 0);
}

static struct NpcState *active_enemy_npc(struct GameState *game)
{
    int slot;

    /* Enemy dialogue resolves through the roster slot, not GameState globals. */
    slot = npc_find_by_dialogue(game, DIALOGUE_ENEMY);
    if (slot < 0) {
        return 0;
    }
    return &game->npcs[slot];
}

/* Bandit open path: npc claims roster slot and queues ENCOUNTER open; genc owns reply/give. */
void enemy_begin_encounter(struct GameState *game, GameEventQueue *out)
{
    if (npc_fixed_begin_encounter_in_room(game, game->player.room_id, out)) {
        game->enemy_handover_pick = 0;
        return;
    }
    if (npc_begin_encounter(game, GAME_DIALOGUE_ACTOR_BANDIT, DIALOGUE_ENEMY,
            GAME_ENCOUNTER_BANDIT, game->player.room_id, 0, out) >= 0) {
        game->enemy_handover_pick = 0;
    }
}

int genc_cmd_give(struct GameState *game, int item_arg, GameEventQueue *out)
{
    struct NpcState *enemy;

    enemy = active_enemy_npc(game);
    if (game->mode == GAME_MODE_DIALOGUE &&
            game->dialogue == DIALOGUE_ENEMY &&
            enemy != 0 &&
            (enemy->flags & NPC_FLAG_HANDOVER_PICK) != 0) {
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
        npc_end_encounter(game, enemy->actor);
        game_set_mode_explore(game);
        return 1;
    }
    push_encounter(out, GAME_ENCOUNTER_BANDIT, GAME_ENCOUNTER_ACTION_GIVE,
        GAME_ENCOUNTER_OUTCOME_WRONG_CONTEXT, 0, 0);
    return 1;
}

int genc_cmd_reply(struct GameState *game, int choice, GameEventQueue *out)
{
    struct NpcState *enemy;

    enemy = active_enemy_npc(game);
    if (enemy == 0) {
        return 0;
    }
    /* Reply choice decides whether the enemy opens combat, asks for tribute, or reacts to intimidation. */
    if (choice == 1) {
        enemy->flags &= ~NPC_FLAG_HANDOVER_PICK;
        combat_start(game, out);
        return 1;
    }
    if (choice == 2) {
        if (game->bag_count <= 0 && game->weapon_equipped == ITEM_NONE) {
            push_encounter(out, GAME_ENCOUNTER_BANDIT,
                GAME_ENCOUNTER_ACTION_HANDOVER,
                GAME_ENCOUNTER_OUTCOME_BAG_EMPTY, 0, 0);
            enemy->flags &= ~NPC_FLAG_HANDOVER_PICK;
            combat_start(game, out);
            return 1;
        }
        enemy->flags |= NPC_FLAG_HANDOVER_PICK;
        /* save mirror; mode guards and genc read the slot flag */
        game->enemy_handover_pick = 1;
        push_encounter(out, GAME_ENCOUNTER_BANDIT,
            GAME_ENCOUNTER_ACTION_HANDOVER_PROMPT,
            GAME_ENCOUNTER_OUTCOME_NONE, 0, 0);
        return 1;
    }
    if (choice == 3) {
        enemy->flags &= ~NPC_FLAG_HANDOVER_PICK;
        game->enemy_handover_pick = 0;
        if (game_roll_percent(game) < CFG_BANDIT_INTIMIDATE_SUCCESS_BELOW) {
            push_encounter(out, GAME_ENCOUNTER_BANDIT,
                GAME_ENCOUNTER_ACTION_INTIMIDATE,
                GAME_ENCOUNTER_OUTCOME_SUCCESS, 0, 0);
            npc_end_encounter(game, enemy->actor);
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
