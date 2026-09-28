# Quest and save state (vertical slice foundation)

`config_files/story1.json` defines quests. The save contains only mutable quest
states and current steps; the loader verifies quest and step IDs before restoring.
The first saved-game version is 1. Saves live under the user's application data
directory (`%LOCALAPPDATA%/HeavenHell/save.json` on Windows,
`$XDG_DATA_HOME/heavenhell/save.json` or `~/.local/share/heavenhell/save.json`
on Linux). `config_files/game_save.json` remains a legacy sample, not the active
save destination. New Game ignores any existing save; Continue reads it.

## Quest event rules

- Events first reach StoryManager, then optional EventBus listeners.
- EventBus subscriptions match both event type and subject.
- Only an active quest consumes an event. Each event can advance one step once.
- Area entry and entity spawning are live events only: a previous contact does
  not satisfy a quest activated later.
- Discrete actions (item pickup, kill, drain, possession, dialogue completion,
  and flag change) can satisfy a quest activated after they occurred. At most one
  unmatched action of each type and subject is kept, capped at 128. Consumed
  actions are never replayed or saved.
- A quest's authoring schema is unchanged. `isStoryFinished()` still signals
  when the final quest completes and no other quest is active.

The existing `DialogueFinished` event is emitted when an NPC interaction opens
its one-line textbox, not after the textbox disappears. The current
`ItemPickedUp` event covers inventory pickups but not currency pickups. Quest
content depending on either distinction needs a dedicated event at the actual
gameplay transition.

## World snapshot

The save stores player definition/host, position, health and inventory;
quest state; the selected level layout and its placements; removed layout
entities; surviving layout entities' positions and health; and loose world
items. Layout indices are stable only while the placement list stays identical.
If it changes, Continue rejects the save rather than applying it to the wrong
entities. A possessed layout entity is restored as the player, never spawned
again as an NPC. Loading restores state before gameplay updates can emit events.

This is a slice-specific snapshot, not arbitrary ECS serialization. Temporary
visual effects, NPC inventories, and arbitrary new runtime entities do not
persist. Add explicit state for any new persistent world mechanic when it is
introduced. Save writes use a temporary file and replacement so failure does
not truncate the previous save. Unknown versions or changed quest definitions
are rejected rather than silently resetting progression.
