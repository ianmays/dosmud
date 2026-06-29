# DOSMUD Roadmap v2

## Current phase

**Playable authored adventure spine.**

Roadmap v1 established the engine foundation:

- ANSI C89 / OpenWatcom-compatible architecture discipline
- deterministic testing and replay support
- GameEvent output boundary
- NPC roster / authored placement foundations
- save/load
- authored content and engine IoC platform seams
- first authored narrative slice ([#76](https://github.com/ianmays/dosmud/issues/76))

Roadmap v2 starts from that foundation and focuses on making authored adventure content repeatable without building premature frameworks.

## Core project rules

Keep these visible from v1:

```text
Language target:
- ANSI C89 / ISO C90
- OpenWatcom compatible
- GCC compatible
- No compiler extensions required
```

Prioritize:

- deterministic gameplay
- procedural ANSI C architecture
- explicit ownership and state flow
- DOS/OpenWatcom portability
- maintainability over rapid expansion
- subsystem isolation
- deterministic testing

Avoid:

- framework-style architecture
- object-emulation patterns
- ECS-style abstractions
- hidden ownership
- platform/render leakage into gameplay
- premature complexity

## Planning rules

[Project board #1](https://github.com/users/ianmays/projects/1) remains the source of truth for:

- **Status**
- **Agent-ready** order
- active execution queue

This file records:

- current phase
- active lanes
- sequencing principles
- parked systems
- links to archive/history

Do not mirror board columns here.

Ticket creation, milestone reconciliation, and board status checks were handled by the Roadmap v2 operationalisation pass.

GitHub milestones group Roadmap v2 lanes (Authored Interaction Spine v1, World Reactivity and Atmosphere v1, Playability and Text UX v1, Deferred / Long-term) plus existing Renderer, Multiplayer, and Workflow and Tooling Maturity buckets. Milestones are reporting groupings, not board columns.

## Active authored-content spine

Near-term soft sequence:

```text
#76 first authored narrative slice - Done
#132 npc item exchange and rewards
#49 quest/progress helper extraction
#8 dialogue branching and action follow-ups
#220 authored world advancement hooks v1
```

Principle:

```text
Content pressure first.
Reusable seams second.
Frameworks last.
```

- [#76](https://github.com/ianmays/dosmud/issues/76) proved one authored world hook locally (Done - [PR #217](https://github.com/ianmays/dosmud/pull/217)).
- [#132](https://github.com/ianmays/dosmud/issues/132) proves NPC item exchange/rewards.
- [#49](https://github.com/ianmays/dosmud/issues/49) extracts helpers from proven content.
- [#8](https://github.com/ianmays/dosmud/issues/8) wires choices to existing actions/state changes.
- [#220](https://github.com/ianmays/dosmud/issues/220) generalises persistent authored world advancement hooks.

## Active lanes

### Lane 1 - Authored interaction spine

**Purpose:** Make authored story interaction repeatable without building a generic quest engine.

**Contains / currently expected:**

- [#132](https://github.com/ianmays/dosmud/issues/132) npc item exchange and rewards
- [#49](https://github.com/ianmays/dosmud/issues/49) quest/progress helper extraction
- [#8](https://github.com/ianmays/dosmud/issues/8) dialogue branching and action follow-ups
- [#220](https://github.com/ianmays/dosmud/issues/220) authored world advancement hooks v1

**Out of scope for this lane:**

- full economy ([#50](https://github.com/ianmays/dosmud/issues/50))
- generic quest framework
- quest DSL
- external data packs
- arbitrary dialogue graph engine
- cutscene framework
- full NPC schedules ([#52](https://github.com/ianmays/dosmud/issues/52))
- reputation ([#9](https://github.com/ianmays/dosmud/issues/9))

### Lane 2 - World reactivity and atmosphere

**Purpose:** Make rooms and environmental features feel interactive beyond movement/combat/dialogue.

**Contains / currently expected:**

- [#7](https://github.com/ianmays/dosmud/issues/7) ambient world-effect interactivity
- [#51](https://github.com/ianmays/dosmud/issues/51) weather, if still wanted
- [#54](https://github.com/ianmays/dosmud/issues/54) procedural encounters / more encounters
- [#130](https://github.com/ianmays/dosmud/issues/130) night time, if kept small
- future authored environmental consequences if needed

This lane is separate from persistent authored world advancement hooks ([#220](https://github.com/ianmays/dosmud/issues/220)).

### Lane 3 - Playability and text-output discipline

**Purpose:** Make the current text game easier to play, read, fail, recover, and navigate.

**Contains / currently expected:**

- [#206](https://github.com/ianmays/dosmud/issues/206) player defeat/respawn flow
- [#145](https://github.com/ianmays/dosmud/issues/145) local map viewport

### Lane 4 - Tooling and showcase

**Purpose:** Improve agent/reviewer visibility and repeatable playtest evidence.

**Contains / currently expected:**

- [#180](https://github.com/ianmays/dosmud/issues/180) asciinema local recording / later Pages playback

Tooling should support delivery. It should not block the authored-content spine unless explicitly chosen.

### Lane 5 - Presentation platform

**Purpose:** Alternative front-end/presentation work.

**Contains / currently expected:**

- [#48](https://github.com/ianmays/dosmud/issues/48) SDL renderer
- [#207](https://github.com/ianmays/dosmud/issues/207) 25-line safe output mode (Renderer milestone; near-term text-output/playability work)

SDL is parked/later unless presentation work is explicitly prioritised. It must not own gameplay, simulation, combat logic, or world state. [#207](https://github.com/ianmays/dosmud/issues/207) stays on the Renderer milestone but is near-term playability/text-output discipline within this lane.

### Lane 6 - Long-term platform vision

**Purpose:** Large architecture direction beyond the current single-player authored spine.

**Contains / currently expected:**

- [#92](https://github.com/ianmays/dosmud/issues/92) multiplayer

Multiplayer remains parked until the single-player authored spine is stronger. It should later be split into design/spike/protocol/server/client issues before implementation.

## Parked/deferred systems

Parked unless current authored content creates real pressure:

- [#50](https://github.com/ianmays/dosmud/issues/50) economy
- [#52](https://github.com/ianmays/dosmud/issues/52) NPC availability and schedule v1
- [#9](https://github.com/ianmays/dosmud/issues/9) reputation
- [#15](https://github.com/ianmays/dosmud/issues/15) stats and rolls
- [#31](https://github.com/ianmays/dosmud/issues/31) easy / hard mode
- [#55](https://github.com/ianmays/dosmud/issues/55) larger worlds
- [#131](https://github.com/ianmays/dosmud/issues/131) cooking skill
- [#221](https://github.com/ianmays/dosmud/issues/221) narration triggers v1
- [#222](https://github.com/ianmays/dosmud/issues/222) cutscene-style scene beat v1
- [#223](https://github.com/ianmays/dosmud/issues/223) acts and chapter boundary planning note
- [#224](https://github.com/ianmays/dosmud/issues/224) quest DSL and external data packs decision record
- full merchant/shop loop
- full cutscene framework
- [#48](https://github.com/ianmays/dosmud/issues/48) SDL renderer (see Lane 5)
- [#92](https://github.com/ianmays/dosmud/issues/92) multiplayer (see Lane 6)

Parking does not mean cancelled. It means not part of the current authored-adventure spine.

## Step 3B follow-up tickets (created)

- [#220](https://github.com/ianmays/dosmud/issues/220) authored world advancement hooks v1
- [#221](https://github.com/ianmays/dosmud/issues/221) narration triggers v1
- [#222](https://github.com/ianmays/dosmud/issues/222) cutscene-style scene beat v1
- [#223](https://github.com/ianmays/dosmud/issues/223) acts and chapter boundary planning note
- [#224](https://github.com/ianmays/dosmud/issues/224) quest DSL and external data packs decision record

Ticket F (agent/skill compatibility cleanup) not created: compatibility already covered by PR #219 and no live blocking gaps found.

## Archive and history

Historical plan:

- [docs/archive/DEV_PLAN_v1_engine_foundation.md](docs/archive/DEV_PLAN_v1_engine_foundation.md)

Use the archive when you need detailed issue history, old milestone grouping, or Done-marker history from the engine-foundation phase.
