# Architecture

DOSMUD is a DOS-first adventure/RPG written in ANSI C89.

## Core philosophy

- deterministic simulation for identical seed + inputs
- clear subsystem boundaries
- explicit procedural code over heavy abstraction
- low memory overhead and DOS-friendly constraints

The codebase intentionally avoids:

- dynamic allocation by default
- hidden globals
- object-emulation frameworks and ECS-style designs

## Engine layers (core / render / platform)

The codebase splits into three layers. Gameplay and simulation modules must stay in **core**; only **render** and **platform** may touch terminal I/O or OS APIs.

### Core (gameplay simulation)

Owns state mutation, command handling, world ticks, combat, inventory logic, and procedural systems.

Modules include `game`, `command`, `world`, `invent`, `combat`, `gatmos`, `gprog`, `gstory`, `gwhok`, `genc`, `dialogue`, `npc`, and `items`.

Simulation steps append fixed-size `GameEvent` records through the `gout` queue while mutating `GameState`. Core does not print directly; callers may inspect those records headlessly or hand them to the DOSMUD render adapter.

Within core, keep the ownership split explicit:

- **Engine** - deterministic `GameState` stepping (`game_describe_current_room`, `game_describe_current_room_tight`, `game_process_input`, `game_background_step`) plus `GameEvent` / `gout` records. The engine mutates state and emits semantic output requests, but never performs terminal I/O.
- **Game logic** - dosmud-specific rules and content that plug into that stepping surface: command routing in `game`, room/world rules, and the gameplay slices (`combat`, `invent`, `dialogue`, `npc`, `genc`, `gatmos`, `gprog`, `gstory`, `gwhok`, `items`).

[`src/game.h`](https://github.com/ianmays/dosmud/blob/main/src/game.h) defines the engine-facing stepping surface and persistent simulation state; [`src/gout.h`](https://github.com/ianmays/dosmud/blob/main/src/gout.h) defines the fixed-size event queue that carries engine results to the render edge.

Command and navigation stepping in `game.c` emit generic `GameEventKind` values (handled in `grendr`): `GAME_EVENT_MOVE` and `GAME_EVENT_ROOM_LOOK` (`MOVE` on a successful room change; `ROOM_LOOK` after `advance_world_tick` when the player remains in explore mode, or from slice callers on peaceful modal exit), `GAME_EVENT_MAP`, `GAME_EVENT_HELP` (`arg0` = `CMD_HELP_*` topic), `GAME_EVENT_VERSION` (`text` = shared build-identity line), `GAME_EVENT_WAIT`, `GAME_EVENT_CANNOT_MOVE` (`text` = direction name), and `GAME_EVENT_UNKNOWN_COMMAND`. Inventory and item handlers in `invent.c` emit `GAME_EVENT_ITEM_RESULT`, `GAME_EVENT_CORPSE_VIEW`, `GAME_EVENT_BAG_VIEW`, `GAME_EVENT_CRAFT_RESULT`, and `GAME_EVENT_EQUIP_RESULT` (payload contract in [`gout.h`](https://github.com/ianmays/dosmud/blob/main/src/gout.h)). Combat and progression in [`combat.c`](https://github.com/ianmays/dosmud/blob/main/src/combat.c) and [`gprog.c`](https://github.com/ianmays/dosmud/blob/main/src/gprog.c) emit `GAME_EVENT_COMBAT` (`GameEventCombatPhase` in `arg0`), `GAME_EVENT_XP_GAIN`, and `GAME_EVENT_STAT_CHANGE` (payload contract in [`gout.h`](https://github.com/ianmays/dosmud/blob/main/src/gout.h)). The NPC seam in [`npc.c`](https://github.com/ianmays/dosmud/blob/main/src/npc.c) owns fixed room lookup, shared dialogue event helpers, fixed room-NPC item exchange, and the fixed-size NPC instance roster that carries traveler roaming encounters plus authored enemy profiles such as the road bandit (seeded on the road with authored level data, then opened from roster co-location via `npc_roaming_begin_encounter_in_room`; combat snapshots that level into `GameState.combat` so save/load and XP scaling do not depend on a live roster lookup). Dialogue and encounter slices in [`dialogue.c`](https://github.com/ianmays/dosmud/blob/main/src/dialogue.c), [`npc.c`](https://github.com/ianmays/dosmud/blob/main/src/npc.c), and [`genc.c`](https://github.com/ianmays/dosmud/blob/main/src/genc.c) emit `GAME_EVENT_DIALOGUE`, `GAME_EVENT_ENCOUNTER`, and `GAME_EVENT_DIALOGUE_GUARD`; modal guards in `game.c` emit `GAME_EVENT_DIALOGUE_GUARD` when reply, room-NPC give, or handover context blocks a command (payload contract in [`gout.h`](https://github.com/ianmays/dosmud/blob/main/src/gout.h)). Ambient and inspect output in [`gatmos.c`](https://github.com/ianmays/dosmud/blob/main/src/gatmos.c) emit `GAME_EVENT_ENVIRONMENT`, `GAME_EVENT_AMBIENT_NOISE`, `GAME_EVENT_ITEM_PRESENCE`, and `GAME_EVENT_OBSERVATION` on world ticks and the `inspect` command; successful inspect of an active clue clears that clue bit in `env_room_clues[]` and opens a numbered follow-up menu via `GAME_EVENT_ENV_MENU`, with reply outcomes on `GAME_EVENT_ENV_RESULT` ([#7](https://github.com/ianmays/dosmud/issues/7); payload contract in [`gout.h`](https://github.com/ianmays/dosmud/blob/main/src/gout.h)). Global weather ([#51](https://github.com/ianmays/dosmud/issues/51)) and day/night ([#130](https://github.com/ianmays/dosmud/issues/130)) live in `gatmos.c`: `gatmos_weather_tick` rolls rain/fog/wind/clear from a seed-and-tick hash (not `plat_rand()`), queues weather transition `GAME_EVENT_ENVIRONMENT` subkinds when `weather_kind` changes, biases atmosphere gust/rustle/water/grit thresholds while rain or wind is active, tightens animal-noise skip rolls during fog, and exposes `gatmos_weather_blocks_roaming_encounter` so fog can suppress roster roaming encounter checks; `gatmos_daynight_tick` alternates `day_phase` on expiry with `GAME_ENV_EVENT_NIGHT_FALL` / `GAME_ENV_EVENT_DAY_BREAK`, dawn clears `night_lost`, and `gatmos_try_night_lost_on_move` hash-rolls on successful night moves when the player lacks a torch (carried or wielded). All production slices append generic `GameEventKind` values only; `grendr` dispatches them to player-visible text.

Core must **not** use:

- `printf`, `render_*`, or other terminal output APIs
- DOS, SDL, or platform timing/input APIs
- `#ifdef __WATCOMC__` or similar platform switches

Good:

```c
GameEventQueue out;

game_event_queue_reset(&out);
game_process_input(game, line, &out);
game_render_output(game, &out);
```

Bad in core:

```c
printf("Player moved.\n");
```

### Render (`txtres`, `fmt`, `grendr`)

Presentation only: room art, HUD, combat text, inventory messages, and exploration map output.

- **`txtres`** holds static copy and stable dialogue/encounter event-to-scene key tables (`TxtresNarrativeKey`, `txtres_dialogue_narrative_key`, `txtres_encounter_narrative_key`); it does not print
- **`fmt`** builds player-visible strings from `GameState` into caller buffers (no terminal I/O); logic-heavy formatting (for example aggregated bag lists) lives here
- **`grendr`** is the only gameplay-adjacent module that may call `printf`; it prints `fmt` output, applies newline/spacing tiers, draws ASCII art, and acts as the DOSMUD text-render adapter over the generic `GameEvent` queue (room/move, command/navigation, inventory/item, combat/progression, dialogue/encounter, ambient/inspect, post-inspect env menus). For `GAME_EVENT_DIALOGUE` and `GAME_EVENT_ENCOUNTER`, it resolves a `TxtresNarrativeKey` through `txtres` and dispatches to the matching `render_*` helper so authored scenes can grow without reopening actor/phase switch ladders in the render path.

Platform or frontend code runs simulation first, then hands the resulting `GameEvent` records to render; render never changes simulation state.

The optional replay log path stays outside render. In all builds, `main.c` may mirror each per-step `GameEventQueue` into [`src/replay.c`](https://github.com/ianmays/dosmud/blob/main/src/replay.c) before the next queue reset when `--replay-log` is active, but `grendr` remains the only text renderer and the replay log remains a separate persistent record.

The save/load path also stays outside render and gameplay slices. [`src/save.c`](https://github.com/ianmays/dosmud/blob/main/src/save.c) serializes the durable `GameState` snapshot at the shell edge, while `main.c` owns the `save` / `load` commands, success/error copy, and post-load room redraw. Because `main.c` intercepts `save` / `load` before `game_process_input`, they bypass the modal resolver and never advance world time.

### Newline and spacing

Player-facing copy lives in `txtres`; `grendr` owns when a blank line appears before output.

- **`txtres`:** each string ends with exactly one `\n`. Do not embed a leading `\n` in copy (the main prompt in `main` is the exception).
- **`grendr`:** use the tier helpers in [`grendr.c`](https://github.com/ianmays/dosmud/blob/main/src/grendr.c):
  - **Line** - inline messages (combat, errors): print copy only.
  - **Flavor stack** - tick `GAME_EVENT_ENVIRONMENT` / `GAME_EVENT_AMBIENT_NOISE` stack-tier lines merge into one sentence in `atmo_stack` and flush before non-flavor events ([#236](https://github.com/ianmays/dosmud/issues/236)).
  - **Scene** - room look art, encounters, NPC portraits: `render_gap()` once before the art block; `render_copy()` passes tight copy without an extra gap.
  - **Look footer** - `GAME_EVENT_ROOM_LOOK` appends weather, night, and active clue phrases to the room description in one block (`txtres_look_clue_phrase`, `txtres_look_weather_phrase`; `GAME_ROOM_LOOK_FLAG_SUPPRESS_WEATHER` skips weather when the same step already announced a transition).
  - **Compact arrival/return** - `MOVE` or peaceful `ENCOUNTER` reply followed by flavor plus trailing `ROOM_LOOK` coalesces into one render block when `GAME_ROOM_LOOK_FLAG_TIGHT_LEAD` is set ([#236](https://github.com/ianmays/dosmud/issues/236)).
- **ASCII art:** the first row must be drawing, not a blank spacer row; room captions print on one line after the art with no trailing blank spacer rows. Section breaks come from `render_gap()`, not padding lines inside art.

### Platform (`main`, `platform.h`, `platdos.c` / `platpos.c` / `platwin.c`)

[`include/platform.h`](https://github.com/ianmays/dosmud/blob/main/include/platform.h) defines the portable boundary:

- `plat_poll_line` - non-blocking stdin poll (DOS `kbhit`/`getch`, Windows console `_kbhit`/`_getch`, or POSIX `select`)
- `plat_time_now` - wall-clock seconds for idle ticks
- `plat_seed_rng` - applies `srand((unsigned int)seed)`; `main.c` chooses a `u32` seed (`CFG_DEFAULT_RAND_SEED`, `--seed <unsigned>`, or `--seed wallclock`). `GameState.seed` stores the full `u32`; libc may use fewer bits (for example 16-bit `unsigned int` on DOS)
- `plat_rand` plus `plat_rand_draw_count` / `plat_rand_advance` - tracked libc RNG draws so save/load can restore the future random stream without serializing libc internals

Implementations are split by toolchain (FAT 8.3 basenames):

- [`src/platdos.c`](https://github.com/ianmays/dosmud/blob/main/src/platdos.c) - Open Watcom / DOS (`build.bat` links `platdos.obj`)
- [`src/platpos.c`](https://github.com/ianmays/dosmud/blob/main/src/platpos.c) - GCC / POSIX (`Makefile` links `platpos.c`)
- [`src/platwin.c`](https://github.com/ianmays/dosmud/blob/main/src/platwin.c) - Windows console path for WSL cross-builds (`make build-win` / `make test-win`)

[`src/main.c`](https://github.com/ianmays/dosmud/blob/main/src/main.c) orchestrates the main loop and may use `printf` for shell-level prompts and banners. It must not include `conio.h`, `dos.h`, or other platform headers directly.

In all builds, `main.c` accepts `--replay-log [path]`, opens a deterministic text log, and records each startup, input, and idle step after simulation produces the queue and before the next reset clears it. The log includes the seed, step index, tick, input text when present, queue overflow state, and serialized `GameEvent` payloads in queue order. If the flag omits a path, logging defaults to `replay.log`.

In all builds, `main.c` accepts `--version` at the shell edge and in-session `save`, `load`, and `version` commands. `--version` prints the shared build identity and exits before startup; the in-session `version` command routes through `GAME_EVENT_VERSION` so gameplay remains print-free. Build identity comes from the checked-in `VERSION` file plus generated native metadata in `build/include/version.h`, with the checked-in `include/version.h` fallback covering DOS/OpenWatcom and other non-generated paths. `save` and `load` still do not advance time, use the single-slot `save.dat` path in the current working directory, serialize the durable simulation state through [`src/save.c`](https://github.com/ianmays/dosmud/blob/main/src/save.c), and redraw the restored room immediately after a successful load.

Run `make check-layers` before opening a PR (or use `make test-all`, which runs it first). That target fails if `printf` appears in any `src/*.c` other than `main.c`, `grendr.c`, `platdos.c`, `platpos.c`, or `platwin.c`. `make test` compiles only and does not run the guard.

## High-level flow

```text
Input
  ->
Command Parsing
  ->
Gameplay Logic
  ->
World Tick Advance
  ->
Rendering
```

Commands mutate game state, world ticks mutate simulation state, and rendering only displays state.

## Configuration (`config.h`)

[`include/config.h`](https://github.com/ianmays/dosmud/blob/main/include/config.h) is the compile-time home for:

- structural limits (`CFG_ROOM_MAX`, `CFG_BAG_MAX`, `CFG_AREA_ITEM_SLOTS` ground slots per room, `CFG_CORPSE_ITEM_SLOTS` per-room corpse loot slots, buffers, etc.)
- gameplay tuning (combat enemy level scaling, bandit corpse loot thresholds, food/salve heal amounts, progression, ambient systems, friendly roaming NPC timing)
- world-generation numeric policy (`world_init` counts and loop bounds)
- `WORLD_ROOM_*` room IDs

Conventions:

- add new gameplay/procedural tuning knobs here as `CFG_*` macros
- keep related values grouped and commented
- separate gameplay tuning from main-loop/test-harness settings
- distinguish bandit corpse loot count (`CFG_COMBAT_CORPSE_LOOT_NONE_BELOW` through `TWO_BELOW`, 0-3 drops) and portable item rolls (`CFG_COMBAT_CORPSE_LOOT_SPEAR_BELOW` and siblings) from ambient room finds (`CFG_ROOM_SPAWN_*`, terrain-driven junk like stone)
- all builds default libc RNG to `CFG_DEFAULT_RAND_SEED` (1234) for deterministic startup; override with `dosmud --seed <unsigned>` or `dosmud --seed wallclock`
- roll-inject limits and snapshot roll constants (`CFG_ROLL_INJECT_*`, `CFG_TEST_*`) are defined only under `#ifdef TEST_MODE` in `config.h`

### Test harness (`testharn`, `TEST_MODE` only)

[`tests/harness/testharn.c`](https://github.com/ianmays/dosmud/blob/main/tests/harness/testharn.c) lives at the `main` edge (not core simulation). It applies `@fixture` and `@seed` lines from snapshot `.input` files by calling `game_reset_fixture_baseline` plus real gameplay APIs, usually capturing the event queue into a local buffer and either dropping it with `harness_drop_output` or rendering it through `game_render_output` when the snapshot needs the visible prompt or encounter text. A few edge prompts still use direct `render_*` calls where the harness is intentionally reproducing shell-facing output that is not part of a normal command or tick step. Shared seed-1234 world layout tables live in [`tests/harness/th_world.c`](https://github.com/ianmays/dosmud/blob/main/tests/harness/th_world.c). After a successful harness directive, `main.c` calls `plat_seed_rng(game.seed)` so libc RNG matches the stored seed. Fixtures cover bandit dialogue/combat, the seeded road-bandit encounter, friendly roaming dialogue (`traveler_dialogue`, `lost_animal_dialogue`, `peddler_dialogue`), room placement, bag contents, inspect focus, corpse loot, combat-ready inject queues, and `quiet_explore` (`test_quiet_ticks` + roaming traveler off). The `bandit_combat_turn1_resolve` fixture also calls `game_roll_inject_begin` and `combat_resolve_reply` so the `equipment` snapshot exercises real combat without a scripted `1`. `@seed` sets `GameState.seed` mid-file for libc RNG stream isolation. Release builds (`make build`) do not link the harness. See [testing](testing.md#test-fixtures-test_mode-only).

## Base types (`base.h`)

[`include/base.h`](https://github.com/ianmays/dosmud/blob/main/include/base.h) defines portable unsigned aliases used in simulation state:

- `u8` - byte storage (explored-map flags, `map_ready`, safe `char` casts for `ctype` calls)
- `u16` - 16-bit values when needed (defined for future use; compile-time `sizeof` guard)
- `u32` - tick counters, seeds, and other monotonic world-time fields (`GameState.tick`, `world_step` argument); maps to `unsigned long` (32-bit on DOS/Open Watcom, may be wider on LP64 Linux hosts)

Conventions:

- use `int` for gameplay quantities, room ids, and boolean-style flags unless a fixed width is required
- keep `config.h` for limits and tuning; do not fold typedefs into `config.h`
- see [contributor guide](contributor-guide.md) for ANSI C89 / Open Watcom portability rules

## Subsystem responsibilities

### `main`

- startup
- main loop orchestration
- input/timing integration
- `--version` CLI (all builds); prints the shared build identity via [`buildid.c`](https://github.com/ianmays/dosmud/blob/main/src/buildid.c) and exits before the game loop
- optional replay log capture via `--replay-log [path]` (all builds)
- `TEST_MODE`: delegates `@fixture` and `@seed` lines to `testharn`
- in-session `save` / `load` shell commands (all builds); intercepts before `game_process_input` so ticks do not advance; after a successful load, calls `gwhok_apply_all` so persisted `world_adv_flags` reconcile authored room copy before redraw ([#220](https://github.com/ianmays/dosmud/issues/220))

### `buildid`

- read-only build identity accessors in [`buildid.c`](https://github.com/ianmays/dosmud/blob/main/src/buildid.c); values come from `version.h` and [`txtres.c`](https://github.com/ianmays/dosmud/blob/main/src/txtres.c) (`TXT_MAIN_VERSION_FMT`)
- shared by `main.c` (`--version`) and `game.c` (`CMD_VERSION` → `GAME_EVENT_VERSION` with `build_version_line()`)
- no game state or RNG mutation

### `replay`

- shell-edge serialization in [`replay.c`](https://github.com/ianmays/dosmud/blob/main/src/replay.c) (all builds); opened and driven from `main.c`
- writes a deterministic sidecar text log (`dosmud-replay-v1`) of startup, input, and idle steps
- captures each step's `GameEventQueue` after simulation and before the next queue reset; does not mutate gameplay or render state
- I/O failure surfaces through `main.c` stderr and exits non-zero

### `save`

- shell-edge binary serialization in [`save.c`](https://github.com/ianmays/dosmud/blob/main/src/save.c); called only from `main.c`
- versioned, field-by-field save format (`DMSV`, version 19) for `GameState`, `World`, and tracked RNG draw count
- loads only the current `SAVE_VERSION`; version 19 replaces the single tick-scoped `env_focus_*` fields with per-room `env_room_clues[CFG_ROOM_MAX]` inspect-clue bit flags ([#234](https://github.com/ianmays/dosmud/issues/234)); version 18 persists `day_phase`, `day_expires_tick`, and `night_lost` for global day/night ([#130](https://github.com/ianmays/dosmud/issues/130)); version 17 widens the NPC roster block to `CFG_NPC_MAX` 8 for lost animal and peddler profiles ([#54](https://github.com/ianmays/dosmud/issues/54)); version 16 persists `weather_kind` and `weather_expires_tick` for global weather ([#51](https://github.com/ianmays/dosmud/issues/51)); version 15 persists `env_interact_*` for the post-inspect ambient menu ([#7](https://github.com/ianmays/dosmud/issues/7)); version 14 persists `world_adv_flags` for authored world advancement hooks ([#220](https://github.com/ianmays/dosmud/issues/220)); version 13 persists session `herbalist_menu` alongside story state; version 12 added composable `watchman_flags` and session `watchman_menu` ([#8](https://github.com/ianmays/dosmud/issues/8)); older files return `SAVE_RESULT_FORMAT` during active development instead of carrying migration paths; successful loads use the persisted roster, combat snapshot, and authored-story room copy as written (no profile upgrade or combat backfill on read); `main.c` reconciles room desc copy from `world_adv_flags` via `gwhok_apply_all` after a successful read
- validates magic, version, and field ranges into a staging buffer before replacing the live game state; failed loads leave the caller's `GameState` untouched
- fixed-size NPC roster (`GameState.npcs[]` with actor, dialogue, encounter, level, room, flags, and return tick per slot); `CombatState.enemy_level` snapshots the active encounter level for in-combat save/load; enemy handover-pick state lives on `NPC_FLAG_HANDOVER_PICK` on the active enemy slot; `TEST_MODE` builds append roll-injection and quiet-tick fields after the shared payload, while release builds use the shorter record and reject extra trailing bytes
- keeps render queues and replay logs out of the save format

### `game`

- top-level gameplay orchestration
- command routing
- world update sequencing in `advance_world_tick`: increment tick, run `gatmos_weather_tick`, then `gatmos_daynight_tick` ([#130](https://github.com/ianmays/dosmud/issues/130)), then roster maintenance, roaming encounter opens (fog blocks opens only, not movement), roaming steps when no encounter opened yet, fixed-slot encounter opens, then `world_step`; when the tick started busy (`busy_before_tick`, any dialogue/combat/env menu) or co-located roaming or fixed-slot checks set `encounter_opened`, ambient rolls are skipped for that tick so modal waits stay quiet and `MOVE` plus encounter copy stay ahead of deferred `ROOM_LOOK` ([#235](https://github.com/ianmays/dosmud/issues/235)); move and wait share the same order; no player-site random bandit roll on tick
- successful move during night calls `gatmos_try_night_lost_on_move` before `GAME_EVENT_MOVE`; `game_process_input` defers `ROOM_LOOK` until after `advance_world_tick` when the player is still in explore mode ([#130](https://github.com/ianmays/dosmud/issues/130), [#235](https://github.com/ianmays/dosmud/issues/235)); torch in bag or wielded skips the roll; dawn clears `night_lost` via `gatmos_daynight_tick`
- `game_cmd_move` calls `gatmos_clear_departed_room_clues` on the room left behind after a successful move, so uninspected ambient clues do not persist once the player walks away ([#234](https://github.com/ianmays/dosmud/issues/234))
- headless step surface: `game_describe_current_room`, `game_describe_current_room_tight`, `game_process_input`, and `game_background_step` mutate `GameState` and append `GameEvent` records supplied by the caller; `game_describe_current_room` enqueues `ROOM_LOOK` for startup and load; peaceful encounter and roaming reply exits call `game_describe_current_room_tight` so `grendr` can coalesce reply copy with the trailing look ([#236](https://github.com/ianmays/dosmud/issues/236)); `do_look` sets `GAME_ROOM_LOOK_FLAG_SUPPRESS_WEATHER` when weather began on the current tick or the queue already holds a weather transition, and `mark_room_look_weather_suppressed` retrofits that flag onto earlier `ROOM_LOOK` snapshots queued before `advance_world_tick` on move/wait
- explicit game modes in [`game.h`](https://github.com/ianmays/dosmud/blob/main/src/game.h): `GameMode` (explore, dialogue, combat), `DialogueKind` for the active dialogue when in dialogue mode (room NPCs including the pond frog, friendly roaming branches traveler/lost animal/peddler, enemy bandit, and the corpse-loot menu), `CombatState` for combat-only fields (`enemy_hp`, snapshotted `enemy_level`, `defending`), the narrow Herbalist story fields (`herbalist_story`, `herbalist_menu`, `marsh_root_spawned`) for issue [#76](https://github.com/ianmays/dosmud/issues/76), composable `watchman_flags` and session `watchman_menu` for the tower watchman branch ([#8](https://github.com/ianmays/dosmud/issues/8)), `world_adv_flags` for persisted authored world advancement ([#220](https://github.com/ianmays/dosmud/issues/220); reconciled by `gwhok`), and `NpcState` / `NpcFlags` for the fixed-size dynamic NPC roster (`GameState.npcs[CFG_NPC_MAX]`, cap in [`config.h`](https://github.com/ianmays/dosmud/blob/main/include/config.h); per-slot `level` holds authored enemy difficulty; `NPC_FLAG_HANDOVER_PICK` gates enemy give-during-dialogue on the active slot)
- mode transitions via `game_set_mode_explore`, `game_set_mode_dialogue`, and `game_set_mode_combat` (only one major mode at a time)
- `CMD_GIVE` routes `genc_cmd_give` first while enemy handover owns `DIALOGUE_ENEMY`, otherwise tries room-NPC item exchange through `npc_cmd_give`; unmatched gives queue `GAME_DIALOGUE_GUARD` with `GAME_DIALOGUE_GUARD_GIVE_NO_TARGET` ([#132](https://github.com/ianmays/dosmud/issues/132))
- non-enemy dialogue menus and corpse loot menus (`DIALOGUE_LOOT`) resolve through the unified modal policy ([#240](https://github.com/ianmays/dosmud/issues/240), summarized below): numbered `reply`, session verbs (`help`, `version`, `quit`), and self-directed maintenance (`bag`, `wield`, `unwield`, `eat`, `use`, `craft`, `drop`) stay modal (`KEEP`); repeat `talk` keeps a friendly menu open and repeat `loot` keeps the corpse menu open (`KEEP`); world-directed verbs (`look`, `inspect`, `map`, `take`) and `wait` block and replay the active prompt without advancing time (`BLOCK`); `move` and other non-menu verbs close the menu first (`GAME_DIALOGUE_GUARD_DIALOGUE_CLOSED` for dialogue menus, invent leave for loot), then run in explore mode (`CLOSE`) ([#205](https://github.com/ianmays/dosmud/issues/205), [#235](https://github.com/ianmays/dosmud/issues/235))
- modal command policy now routes through a single classifier in `game.c` ([#240](https://github.com/ianmays/dosmud/issues/240)): active combat, enemy dialogue, peaceful dialogue, corpse loot, and env menus each resolve commands to `KEEP`, `BLOCK`, or `CLOSE`, but `apply_command` still dispatches context-owned teardown (peaceful conversation exit, loot leave, env dismiss) instead of flattening slice behavior. Session verbs plus self-directed maintenance (`bag`, `wield`, `unwield`, `eat`, `use`, `craft`, `drop`) stay `KEEP`; world-directed verbs and `wait` stay `BLOCK`; movement and other non-menu verbs `CLOSE` only where the modal owner has an explicit teardown path. Blocked verbs queue the matching `GAME_DIALOGUE_GUARD` and replay the active prompt (`combat_replay_menu`, `genc_replay_active_prompt`, `npc_replay_active_prompt`, corpse view, or env menu restore) without advancing world time. `give` / `offer` stays modal and delegates to the owning slice so bandit handover, watchman food, and herbalist turn-in keep their own item-eligibility rules.
- `game_is_busy_dialogue` returns true whenever `mode != GAME_MODE_EXPLORE` or `env_interact_active` is set (ambient encounters, idle background ticks, post-inspect env menu)
- successful `inspect` of a clue set on the current room clears that clue bit in `env_room_clues[]`, opens `env_interact_*` in `gatmos.c`, and queues `GAME_EVENT_OBSERVATION` plus `GAME_EVENT_ENV_MENU`; numbered `reply` routes to `gatmos_cmd_env_reply` while the menu is open; under the unified modal policy ([#240](https://github.com/ianmays/dosmud/issues/240)) world-directed verbs (`look`, `map`, `inspect`, `take`) and `wait` block and replay the restored env menu with `GAME_DIALOGUE_GUARD_ENV_MENU_WAITING`, while `move`, `talk`, and other non-menu verbs dismiss through `gatmos_env_dismiss` before the verb runs and queue `GAME_DIALOGUE_GUARD_ENV_MENU_CLOSED` ([#7](https://github.com/ianmays/dosmud/issues/7), same dismiss pattern as room-NPC menus in [#205](https://github.com/ianmays/dosmud/issues/205))
- `game_roll_spread` and `game_roll_percent` centralize gameplay draws used for combat, corpse loot, kill XP, and bandit intimidate (`plat_rand()` when inject is inactive; inject bypasses the draw counter in `TEST_MODE`)
- `TEST_MODE` only: `game_roll_inject_*` and `test_quiet_ticks` on `GameState`; when `test_quiet_ticks` is set, `advance_world_tick` skips ambient atmosphere, animal noise, enemy encounter checks, and roster-driven roaming NPC movement (see [quiet ticks](testing.md#quiet-ticks-test_quiet_ticks-test_mode-only))
- ambient, roster-driven roaming NPCs, and world generation randomness flow through tracked `plat_rand()` so save/load can restore future deterministic draws

Gameplay slices live beside `game.c` as plain C translation units (no extra framework):

- [`gprog.c`](https://github.com/ianmays/dosmud/blob/main/src/gprog.c) - XP and level-up rewards (`game_xp_to_next_level`, `progression_gain_xp`, `progression_enemy_xp_reward`, `progression_gain_enemy_xp`); queues `GAME_EVENT_XP_GAIN` and `GAME_EVENT_STAT_CHANGE` via `gout` (FAT 8.3-safe basename)
- [`gstory.c`](https://github.com/ianmays/dosmud/blob/main/src/gstory.c) - reusable fetch-quest progress helpers ([#49](https://github.com/ianmays/dosmud/issues/49)); `story_fetch_scene` maps persisted progress plus required-item presence to derived talk/reply scenes (`StoryFetchScene`), `story_world_has_item` scans bag and room ground slots, and `story_seed_recoverable_item` places a quest item when it is not already reachable (optional `spawned_out` marks in-play state); authored NPC slices keep exchange routing, world hooks, and dialogue transitions while calling these helpers (Herbalist marsh-root seeding and scene derivation for [#76](https://github.com/ianmays/dosmud/issues/76); FAT 8.3-safe basename)
- [`gwhok.c`](https://github.com/ianmays/dosmud/blob/main/src/gwhok.c) - table-driven persisted world advancement hooks ([#220](https://github.com/ianmays/dosmud/issues/220)); `world_adv_flags` on `GameState` (save v14+) maps authorship bits to world consequences through private `gwhok_rows[]` filled once in `gwhok_rows_init()` (C89 cannot static-initialize `txtres` pointers); `gwhok_has`, `gwhok_set` (idempotent; first set calls `gwhok_apply_all`), and `gwhok_apply_all` reconcile room `desc` copy from row `done_desc` or baseline `g_room_descs`; triggers live in `npc.c` (Herbalist orchard restore, watchman meal); `main.c` calls `gwhok_apply_all` after successful load; stays free of `gout` and render seams (FAT 8.3-safe basename)
- [`combat.c`](https://github.com/ianmays/dosmud/blob/main/src/combat.c) - combat start (with `CombatInitiator`: opening player or enemy strike before the menu), player reply resolution, enemy turn, and enemy cleanup after victory; snapshots enemy level from the active `DIALOGUE_ENEMY` slot into `CombatState.enemy_level` and scales enemy HP, enemy turn damage, and kill XP via config per-level constants; queues `GAME_EVENT_COMBAT` phases via `gout`; `combat_replay_menu` re-queues `GAME_COMBAT_PHASE_MENU` when a blocked modal verb would otherwise hide the combat menu ([#235](https://github.com/ianmays/dosmud/issues/235)); rolls a weighted 0-3 corpse drop count plus per-item portable loot into corpse slots on victory (randomness via `game_roll_spread` / `game_roll_percent`, not direct `plat_rand()` calls)
- [`invent.c`](https://github.com/ianmays/dosmud/blob/main/src/invent.c) - bag, ground, and corpse inventory ownership; `loot` opens a narrow corpse menu through `GAME_EVENT_CORPSE_VIEW`, `loot all` drains visible corpse slots in order until the bag fills or the body empties, numbered replies still take or leave items without a generic UI framework, and shared helpers own carried-item removal plus the low-level bag/ground writes used by room-NPC reward storage
- [`npc.c`](https://github.com/ianmays/dosmud/blob/main/src/npc.c) - fixed NPC identity seam, shared dialogue helpers, room look hint ownership, the parallel `NPC_ROOM_INFO[]` room-talk table (talk opens by player `room_id`, reply resolves by `game.dialogue` through `npc_open_room_dialogue` / `npc_room_cmd_reply`; `give` / `offer` to room NPCs route through `npc_cmd_give` only when enemy handover does not own the command; `npc_replay_active_prompt` re-shows the active friendly dialogue menu after blocked observe verbs; kept separate from `NPC_PROFILES[]` per [#197](https://github.com/ianmays/dosmud/issues/197)), the narrow Herbalist authored-story slice for [#76](https://github.com/ianmays/dosmud/issues/76) plus the reusable authored item-exchange / reward seam from [#132](https://github.com/ianmays/dosmud/issues/132) (story transitions, world advancement triggers via `gwhok_set` / `gwhok_apply_all` for orchard room copy ([#220](https://github.com/ianmays/dosmud/issues/220)), REQUESTED root talk menu (`herbalist_menu`; gossip choice leaves dialogue) plus in-menu reply+talk chaining like watchman; fetch-quest scene derivation and marsh-root seeding delegate to `gstory` per [#49](https://github.com/ianmays/dosmud/issues/49); accepted/rejected give feedback, npc-owned pre-removal bag/full-ground planning, invent-owned carried-item removal plus salve reward writes), the tower watchman authored branch for [#8](https://github.com/ianmays/dosmud/issues/8) (composable `watchman_flags` / session `watchman_menu`; neutral return-to-root copy from `watchman_flags`; `npc_watchman_give_offer_active` for meal `give`/`offer` and `bag`; warned and meal in-menu threads; meal food via `give` / `offer` only with `WATCHMAN_FLAG_FED`; tower meal sets `WORLD_ADV_TOWER_MEAL` through `gwhok` ([#220](https://github.com/ianmays/dosmud/issues/220)); reply copy keyed through `GAME_EVENT_DIALOGUE` `arg3`), and the fixed-size NPC instance roster for authored profiles plus roaming (`npc_clear_all`, `npc_spawn`, `npc_place`, `npc_move`, `npc_find_*`, `npc_begin_encounter`, `npc_end_encounter`, `npc_deactivate_until`, `npc_seed_profiles`, `npc_fixed_begin_encounter_in_room`, `npc_roaming_*`, `npc_is_roaming_friendly_dialogue`, `npc_roaming_cmd_reply`; `NPC_PROFILES[]` holds traveler, three roaming bandit profiles (road, bridge, canyon), lost animal (meadow), and peddler (grove) with authored `level_min`/`level_max`, roam warmup, and respawn timing; friendly roaming reply branches share `npc_roaming_cmd_reply` (resolve-on-dialogue respawn like traveler) and enqueue `ROOM_LOOK` via `game_describe_current_room_tight` after reply copy ([#235](https://github.com/ianmays/dosmud/issues/235), [#236](https://github.com/ianmays/dosmud/issues/236)); bandits open combat through `npc_roaming_begin_encounter_in_room` and respawn via `npc_end_encounter`; `CFG_NPC_MAX` is 8 for all six seeded profiles; `npc_fixed_begin_encounter_in_room` remains for future non-roaming authored slots; slot order is save/tick-stable)
- [`genc.c`](https://github.com/ianmays/dosmud/blob/main/src/genc.c) - encounter handler registry and bandit gameplay on roster-backed enemy slots; `genc_cmd_reply` and `genc_cmd_give` dispatch through a static `EncounterHandler` table keyed by `NpcState.encounter` (`GAME_ENCOUNTER_*` in [`gout.h`](https://github.com/ianmays/dosmud/blob/main/src/gout.h)); bandit is the first populated row; fight reply opens player-initiative combat, intimidate-fail and bag-empty open enemy-initiative ([#4](https://github.com/ianmays/dosmud/issues/4)); `genc_replay_active_prompt` re-shows open or handover encounter copy after blocked modal verbs ([#235](https://github.com/ianmays/dosmud/issues/235)); peaceful bandit give and intimidate success call `game_describe_current_room_tight` after encounter resolution ([#236](https://github.com/ianmays/dosmud/issues/236)); unhandled kinds return 0 from reply (caller may fall through) and emit bandit `GIVE` `WRONG_CONTEXT` from give; `enemy_begin_encounter` prefers roster co-location (`npc_roaming_begin_encounter_in_room`, then `npc_fixed_begin_encounter_in_room`) before dynamic ambush spawn for explicit callers and tests (FAT 8.3-safe basename)
- [`dialogue.c`](https://github.com/ianmays/dosmud/blob/main/src/dialogue.c) - `talk` guards (including repeat `talk` during friendly roaming dialogue via `npc_is_roaming_friendly_dialogue`), nobody hint, and room-talk open; `reply` delegates fixed room-NPC branches to `npc_room_cmd_reply` while direct room-NPC item exchange stays in `npc_cmd_give` (enemy and corpse-loot reply handling stay in `genc.c` / `invent.c`; friendly roaming replies route through `npc_roaming_cmd_reply` in `game.c`)
- [`gatmos.c`](https://github.com/ianmays/dosmud/blob/main/src/gatmos.c) - initial room items, ambient rolls, animal noise, per-room inspect clue hooks ([#7](https://github.com/ianmays/dosmud/issues/7), [#234](https://github.com/ianmays/dosmud/issues/234)), post-inspect follow-up menus ([#7](https://github.com/ianmays/dosmud/issues/7)), global weather ([#51](https://github.com/ianmays/dosmud/issues/51)), and day/night ([#130](https://github.com/ianmays/dosmud/issues/130)): `weather_kind` / `weather_expires_tick` on `GameState` (save v16+); `gatmos_weather_tick` schedules hash-only kind rolls (32-bit masked multiply/xor for DOS/Linux parity) and emits transition `GAME_ENV_EVENT_WEATHER_*` on change; rain and wind bias atmosphere thresholds; fog lowers animal-noise skip threshold and can block roaming encounter rolls via `gatmos_weather_blocks_roaming_encounter`; `day_phase` / `day_expires_tick` / `night_lost` (save v18+); `gatmos_daynight_tick` alternates day and night on expiry with `GAME_ENV_EVENT_NIGHT_FALL` / `GAME_ENV_EVENT_DAY_BREAK` (dawn clears `night_lost`); `gatmos_try_night_lost_on_move` hash-rolls on successful night moves without a torch and may set `night_lost` plus queue `GAME_ENV_EVENT_NIGHT_LOST`; `gatmos_night_map_blanked` is true when `night_lost` is set and the player lacks a torch; `gatmos_night_torch_lights_map` drives torch illumination copy on `map`; ambient rolls accumulate per-room inspect clue bits in `env_room_clues[CFG_ROOM_MAX]` (save v19+, one bit per `GAME_ENV_*` kind, replacing the single tick-scoped focus; `#234`); `gatmos_cmd_inspect` inspects a named clue kind or, with no argument, the room's sole active clue, clearing that bit and opening `env_interact_*`; `gatmos_clear_departed_room_clues` clears all of a room's uninspected clue bits when the player moves away from it, and encounter-open paths clear the current room's clue bits so stale atmosphere hints do not survive into modal encounter copy; `gatmos_cmd_env_reply` resolves numbered choices (water: follow/drink/step back with optional heal via `game_heal_player`; rustle/creak: ground spawn or leave; grit: flavor-only); `gatmos_env_dismiss` clears the menu and queues `GAME_DIALOGUE_GUARD_ENV_MENU_CLOSED` (FAT 8.3-safe basename)

When a slice exposes new command or state entry points, add tests in the matching `tests/unit/unit_*.c` file (see [When to add or update tests](testing.md#when-to-add-or-update-tests)).

New `src/*.c` and `src/*.h` basenames must stay within **classic FAT 8+3** (at most eight characters before `.c` or `.h`) so MS-DOS 5.x-6.x style volumes and the Open Watcom DOS build can open them reliably. Existing examples: `grendr.*`, `invent.*`, `gprog.*`, `gstory.*`, `gwhok.*`, `gatmos.*`, `genc.*`.

`game` stays orchestration; new behaviour should land in the owning slice above rather than re-centralising into `game.c`. [`game_heal_player`](https://github.com/ianmays/dosmud/blob/main/src/game.c) applies capped HP heals and is used by inventory eat/salve and combat salve reply 3.

### `command`

- parse raw text into structured commands
- keep parsing separate from execution/mutation
- `give` and `offer` both parse as `CMD_GIVE` with an item word ([#132](https://github.com/ianmays/dosmud/issues/132))
- recognizes `save` and `load` tokens (`CMD_SAVE`, `CMD_LOAD`); `main.c` handles file I/O before gameplay mutation
- recognizes `version` (`CMD_VERSION`); `game.c` emits `GAME_EVENT_VERSION` so render stays print-free

### `world`

- room graph data and connectivity
- procedural world generation
- movement validation
- logical map coordinates assigned when rooms are linked (used only for the exploration map display)

### `grendr`

- text rendering only
- art is intentionally compact to work well with 25 line displays (DOS standard)
- no gameplay mutation
- calls [`fmt.c`](https://github.com/ianmays/dosmud/blob/main/src/fmt.c) for logic-heavy strings, then prints; static copy from [`txtres.c`](https://github.com/ianmays/dosmud/blob/main/src/txtres.c) (`TXT_*` constants and `g_room_*` arrays), not scattered literals
- `GAME_EVENT_DIALOGUE` and `GAME_EVENT_ENCOUNTER`: resolve `TxtresNarrativeKey` via `txtres`, then call the portrait/menu or encounter `render_*` helper for that key; `GAME_EVENT_DIALOGUE` `arg3` carries authored scene detail when talk/reply/give branches need more than choice alone (Herbalist story and exchange scenes for [#76](https://github.com/ianmays/dosmud/issues/76) and [#132](https://github.com/ianmays/dosmud/issues/132); watchman and Herbalist in-menu talk menus (story root scenes with portrait; intermediate sub-menus options-only; return-to-root after non-leave replies) and reply outcomes for [#8](https://github.com/ianmays/dosmud/issues/8) and [#76](https://github.com/ianmays/dosmud/issues/76))
- `GAME_EVENT_ENV_MENU` and `GAME_EVENT_ENV_RESULT`: post-inspect ambient follow-up menus and numbered reply outcomes ([#7](https://github.com/ianmays/dosmud/issues/7)); `GAME_EVENT_DIALOGUE_GUARD` reasons include bandit waiting/handover guards, loot waiting, `GAME_DIALOGUE_GUARD_DIALOGUE_CLOSED` when #205 dismisses a non-enemy dialogue menu, and `GAME_DIALOGUE_GUARD_ENV_MENU_CLOSED` when an explore verb dismisses the post-inspect menu before running
- room look and `GAME_EVENT_ENVIRONMENT`: `GAME_EVENT_ROOM_LOOK` arg1 packs corpse (bit 0), snapshotted `weather_kind` (bits 1-2), and `day_phase` (bit 3) at enqueue time ([#51](https://github.com/ianmays/dosmud/issues/51), [#130](https://github.com/ianmays/dosmud/issues/130)); arg2 carries the room's `env_room_clues` bitmask ([#234](https://github.com/ianmays/dosmud/issues/234)); arg3 carries `GameEventRoomLookFlags` (`SUPPRESS_WEATHER`, `TIGHT_LEAD`; [#236](https://github.com/ianmays/dosmud/issues/236)); weather, night, and active clue phrases fold into one look footer appended to the room description; clue kinds owned by the footer and weather/night transition subkinds skip standalone `ENVIRONMENT` render when folded into look or compact arrival/return blocks; stack-tier tick flavor (gust, clue primaries, weather/night transitions, and related lines) batches through `atmo_stack` before flush; `render_exploration_map` consults `gatmos_night_map_blanked` (`night_lost` without torch) for blank map copy plus current-room exits, `gatmos_night_torch_lights_map` for torch illumination copy, then the normal grid
- newline tiers (`render_gap`, `atmo_stack`, look footer, and related rules): [Newline and spacing](#newline-and-spacing)

### `fmt`

- pure string formatting from `GameState`; writes into caller-provided buffers
- no `printf` or gameplay mutation
- exploration map (`fmt_exploration_map`): header, visited grid (or none-explored line), legend, then open exits for the player's current room (label and direction order match `look` in `grendr`)
- unit-tested directly in [`tests/unit/unit_fmt.c`](https://github.com/ianmays/dosmud/blob/main/tests/unit/unit_fmt.c) (see [`docs/testing.md`](testing.md))

### `txtres`

- single home for static player-facing copy
- trailing `\n` only on copy strings; see [Newline and spacing](#newline-and-spacing)
- exported globals, not thin getters
- functions only where selection matters (`txtres_look_clue_phrase`, `txtres_look_weather_phrase` for compact look footer copy; [#236](https://github.com/ianmays/dosmud/issues/236))
- owns stable `TxtresNarrativeKey` tables that map dialogue actor/phase and encounter kind/action/outcome payloads to render-scene keys consumed by `grendr` (m10 narrative indirection; [#196](https://github.com/ianmays/dosmud/issues/196))

### `invent`

- bag/inventory state mutation; command outcomes queue generic `GameEvent` records (`gout`) for `grendr` to render
- per-room corpse loot (`corpse_present[]`, dense `corpse_item[][]` capped by `CFG_CORPSE_ITEM_SLOTS`); bandit defeats can leave a stripped body or up to three loot items; `loot` opens `DIALOGUE_LOOT` for numbered take/leave replies, and `loot all` bulk-drains visible slots in order (menu re-queues on bag-full) without advancing world time
- item use and crafting behavior
- `eat` and inventory `use salve` restore HP via `game_heal_player`; at max HP the item is still consumed but no heal is applied (player sees an already-full message)
- wield/unwield commands track `weapon_equipped` on `GameState`; a wielded weapon is not stored in `bag[]` (it occupies the hand slot only until unwield, drop, or bandit handover moves it)
- `game_inv_remove_carried_item` clears wielded gear before the bag; `game_inv_deliver_room_item` is a shared bag-then-ground helper (unit-tested delivery semantics); authored room-NPC exchanges such as Herbalist plan the reward destination in `npc.c`, then call `game_inv_bag_add` / `game_room_ground_try_add` directly ([#132](https://github.com/ianmays/dosmud/issues/132))
- combat adds `item_weapon_damage_bonus` from `weapon_equipped` when the player attacks; it does not require the weapon id to appear in the bag

### `items`

- item metadata and lookup; `ITEM_MARSH_ROOT` and `marsh-root`/`root` word aliases for the [#76](https://github.com/ianmays/dosmud/issues/76) fetch item
- `item_food_heal_amount` for edible heal values (`CFG_BERRY_HEAL_AMOUNT`, `CFG_FISH_HEAL_AMOUNT`)

## Core data ownership

### `GameState`

`GameState` is the primary simulation container. Gameplay systems should mutate it explicitly and avoid shadow copies. Session fields include `env_room_clues[CFG_ROOM_MAX]` (per-room ambient inspect clue bit flags; save v19+; owned by `gatmos.c`; [#234](https://github.com/ianmays/dosmud/issues/234)), `env_interact_*` (post-inspect menu; save v15+), `weather_kind` / `weather_expires_tick` (global weather; save v16+; owned by `gatmos.c`), and `day_phase` / `day_expires_tick` / `night_lost` (global day/night; save v18+; owned by `gatmos.c`). In `TEST_MODE` builds only, it may include the roll-inject queue and `test_quiet_ticks`; release `GameState` has neither.

### `World` and `Room`

`World` stores room graph data. `Room` stores room metadata, exits, and ambient state.

## Determinism rules

- seed randomness once at startup (`plat_seed_rng` from `main`; gameplay draws use tracked `plat_rand()` via `game_roll_*` or slice calls, with `TEST_MODE` inject bypassing the draw counter); global weather kind, fog encounter-gate rolls, and night-lost-on-move rolls in `gatmos.c` use a deterministic seed/tick hash instead of `plat_rand()` ([#51](https://github.com/ianmays/dosmud/issues/51), [#130](https://github.com/ianmays/dosmud/issues/130))
- evolve simulation via commands and background ticks
- never tie simulation correctness to render cadence
- snapshot tests under `tests/regression/` combine fixtures (direct state), optional roll inject (`TEST_MODE` only), and a fixed default seed for remaining `plat_rand()` paths; see [fixture design trade-offs](testing.md#fixture-design-trade-offs)
- greatest unit tests under `tests/unit/` assert `GameState` and parse results (`make test-unit`); see [unit tests](testing.md#unit-tests-greatest)

## ANSI C89/C90 compatibility rules

Target constraints:

- ANSI C89 / ISO C90
- GCC compatible
- Open Watcom compatible

Allowed/preferred:

- fixed arrays
- explicit structs and procedural flow
- compile-time limits

Avoid:

- C99/C11 features (`//` comments, mixed declarations/statements, declarations inside `for`)
- recursion
- heavy heap allocation unless justified
- compiler-specific extensions

## Feature development guidance

When adding features:

- keep subsystem ownership boundaries clean
- keep deterministic behavior intact
- prefer simple, explicit code over clever abstractions
- preserve ANSI C portability
