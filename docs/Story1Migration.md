# Story1 quest system

`config_files/story1.json` is now the story file used by `Scene_Play`. It replaces the old runtime combination of `story.json` and `quests.json`; those legacy files remain in the repository but are no longer loaded by the game.

## What changed

The old system was a single linear list of integer-indexed quests. It subscribed only to the events known at scene startup, so it could not express branches or react safely when a later quest became active.

The new `StoryManager` loads a named quest graph. Each quest has a string ID, a player-facing `description`, an `initial_state`, one or more steps, and optional progression reactions. Quests are `locked`, `active`, or `completed`.

Only active quests can consume a live event. The first possession after `world.choose_first_face` activates exactly one faction-home quest, while the other three routes remain locked.

## Story1 JSON schema

The current file uses these field names, chosen to resemble `story.json` where possible:

```json
{
  "format": "story-quests-v1",
  "version": 1,
  "quests": [
    {
      "id": "opening.regain_strength",
      "description": "A Small Life",
      "initial_state": "active",
      "steps": [
        {
          "id": "drain_chicken",
          "description": "Drain the chicken to regain strength.",
          "triggers": [
            {"event_type": "EntityDrained", "subjects": ["chicken"]}
          ]
        }
      ],
      "reaction": {
        "type": "ActivateQuest",
        "quest": "opening.leave_battlefield"
      }
    }
  ]
}
```

`triggers` and `subjects` are alternatives: any matching event type and subject completes the current step. Multiple-step quests still advance one step at a time.

`reaction` is the single-action form used by Story1. If a quest or trigger needs several actions later, the loader also accepts a `reactions` array. `reactions_by_trigger` selects an action based on the specific trigger that completed the step; Story1 uses it to select the dwarf, knight, elf, or wizard route.

The loader rejects invalid documents, including unknown events/actions, duplicate quest or step IDs, missing fields, unresolved activation targets, empty triggers, or anything other than exactly one initially active quest.

## Events and progression

Every gameplay event now goes through `Scene_Play::Emit`, which first notifies `StoryManager` and then the optional event bus listeners. The following existing events remain available:

- `ItemPickedUp`
- `EnteredArea`
- `EntityKilled`
- `EntitySpawned`
- `DialogueFinished`
- `FlagChanged`

Story1 adds two events emitted by `Scene_Play::tryPossess`:

- `EntityDrained` fires once when draining finishes and the target's health is transferred.
- `EntityPossessed` fires once after control transfers to the host body.

Both use the affected entity's `CName` as their subject, such as `"chicken"`, `"knight"`, or `"dwarf"`.

Possession remains ungated. To keep an early irreversible action from blocking the opening, StoryManager records unmatched events. When a quest becomes active, it may consume the earliest unused matching historic event. A recorded event is consumed once, so it cannot cascade through several later quests. For example, possessing the knight before draining the chicken later completes the battlefield quest, but it does not also choose the knight faction route.

Completing the final `main.kill_king` quest (currently triggered by killing the placeholder `golem`) ends the story and opens the existing finish scene.

## Dialogue

Dialogue lookup now uses the active quest ID instead of the old English description. `config_files/dialogs.json` has not yet been rewritten with Story1 quest IDs, so NPCs use the existing generic fallback line until new dialogue is authored.

## Faction homes

Story1 uses distinct area subjects:

- `area.dwarf.home`
- `area.knight.home`
- `area.elf.home`
- `area.wizard.home`

The matching entity templates are in `config_files/entities/`. They are spawned from `config_files/mobs.json` at these grid positions:

| Faction | Entity | Position |
| --- | --- | --- |
| Dwarf | `dwarf_home` | `(380, 80)` |
| Knight | `knight_home` | `(480, 80)` |
| Elf | `elf_home` | `(580, 80)` |
| Wizard | `wizard_home` | `(680, 80)` |

The homes share the existing house sprite and collider shape, but each emits its own `EnteredArea` subject. Their X positions are 20 tiles apart.

## Tests

`tests/story/test_story_manager.cpp` covers:

- The full chicken → battlefield → faction → home → final-goal flow.
- Historic early possession being credited exactly once.
- Exclusive faction-route activation for all four hosts.
- Rejection of malformed Story1 data.

Run the suite with:

```bash
cmake --build build/Debug --target sync_assets
ctest --test-dir build/Debug --output-on-failure
```
