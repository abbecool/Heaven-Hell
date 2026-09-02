# Possession and Quest Design

**Status: Design direction.** This records the current working direction for possession and quest structure. It does not settle the final possession rules or establish new in-world facts.

## Core Player Promise

Possession is the central gameplay mechanic. The player should be able to leave a host and possess another suitable individual without an active quest hard-locking, failing, or otherwise disabling that choice.

The player is not playing a fixed dwarf, knight, elf, or wizard protagonist. The player is the demon, and hosts are ways of acting in a living world. A host changes the player's abilities, social access, and available approaches; it should not normally change the underlying world objective or make the story impossible to continue.

The intended long-term scope is that the demon can possess almost any individual in a faction, subject to the eventual possession rules. Quests therefore should not depend permanently on one named dwarf, knight, elf, or wizard.

## Working Possession Model

The exact mechanic is still open, but the following two-part model is a useful direction:

| Concern | Working direction |
|---|---|
| Possession strength | A permanent demon progression value sets the kinds of hosts the demon can overcome. Ordinary faction members should become broadly available; exceptional individuals can require more strength. |
| Essence or energy | A renewable resource can limit how often the demon switches or attempts difficult possessions. Its source, cost, and recovery are still undecided. |
| Host resistance | Health, alertness, will, rank, equipment, training, ritual protection, or local circumstances can make an individual harder to possess. |
| Host value | Individuals can differ within a faction. For example, a very strong dwarf with a heavy hammer, a knight officer with authority, an elf scout with unusual mobility, or a skilled wizard can offer abilities worth the greater possession cost. |

This should make special hosts desirable without making ordinary members of a faction inaccessible. The player needs understandable feedback about why a target cannot yet be possessed and what would make possession possible.

## The Battlefield Guard

The battlefield knight should not be the normal first lasting knight host or the way the knight route is unlocked. The merchant road remains the better place to offer the first lasting dwarf, knight, elf, and wizard hosts.

A likely opening treatment is:

1. The newly awakened demon is too weak to take the healthy, alert, armoured guard as a stable host.
2. Draining the chicken supplies enough immediate strength for a brief intrusion or disorientation, not full possession.
3. The guard resists, is briefly distracted by the demon's presence, and the player escapes the battlefield.

Whether the guard can ever be fully possessed remains open. If it is possible, it should be a temporary or unusual solution rather than an accidental fifth starting route.

## Quest Ownership and Switching

A quest belongs to the world and the demon, not to the body currently controlled by the player. The current host supplies an approach to the quest rather than owning it.

When the player switches host during a quest:

- The switch always succeeds or fails according to the possession mechanic, never because a quest blocks it.
- NPCs, objects, and conflicts continue to exist in the world.
- The player can continue a shared objective through a different faction's abilities when that makes sense.
- If the player leaves a strongly faction-specific situation, its NPCs can continue to an authored later state. Returning later should reveal the consequence or aftermath, never a broken quest.

The goal is not to simulate every faction continuously. Use a small number of deliberate state changes at clear milestones or events instead of a complex real-time quest simulation.

## Quest Types

### Demon and Old City quests

These should normally be usable with any faction. Their objectives concern the demon's survival, ancient evidence, access to the Old City, relics, seals, or the main mystery. Different hosts provide different routes through the same content.

### Shared world events

These have one world outcome but can accept several approaches. A quest can have one journal objective, one destination, and one canonical result without needing four separate faction stories.

For example, a dwarven trade cart needs to reach the mountain settlement. The player might:

- use a dwarf to move the cart or break a blockage;
- use a knight to defend the road or gain passage through a human checkpoint;
- use an elf to scout a safer route or reach a remote mechanism;
- use a wizard to overcome a ward or supernatural obstacle.

The cart reaching the settlement is the same outcome in every case. The player changes the method, not the quest's story.

### Faction-centred situations

These establish what life is like for a group and may be best advanced through that faction's social access or abilities. They do not need to be completable in every host.

For example, a knight escorting an elf prisoner to La Poise is naturally a knight-centred duty. The player may still switch away at any time. The escort can then continue without the player and later become an aftermath: the prisoner reached the city, escaped, was moved, or caused a political response. The player is never forced to remain a knight, but not every host must be able to perform the official escort.

If the intention is for every faction to participate directly, frame the objective more neutrally. For example, "prevent the prisoner's death and stop the border incident from escalating" permits several factions to help while still producing one fixed outcome.

## Automatic Progression Without Fragile Branching

Some opportunities may be missed because the world advances, which can make the factions feel alive. Keep that as an added consequence, not the source of core-story failure.

- Advance background situations only at explicit global milestones, location events, or simple authored triggers.
- Give a quest a small set of states such as `available`, `in progress`, `resolved`, and `aftermath`.
- Preserve essential main-story knowledge, faction access, and required abilities through an available follow-up or alternate source.
- Reserve missed scenes, optional rewards, relationship changes, and the exact first-hand version of an event for the dynamic-world effect.

This keeps the main story coherent even when the player changes bodies frequently, while avoiding a large matrix of branching quest variants.

## Scope-First Rules

For an early playable version:

1. Build one global quest state for each situation, not a separate version per faction.
2. Give a small number of meaningful capability-based interactions at shared locations.
3. Let different hosts alter traversal, combat, access, and short dialogue rather than the central outcome.
4. Use a deterministic off-screen result when a faction-centred event advances without the player.
5. Do not make core progression depend on retaining one named host or completing a narrowly timed quest.

This preserves the freedom and fantasy of possession while keeping the narrative and implementation scope manageable.

## Questions to Decide Later

- What permanent possession-strength measure, if any, governs host eligibility?
- What energy or essence cost governs switching, and how is it recovered?
- Which target properties create resistance: health, will, alertness, rank, equipment, ritual protection, or a combination?
- What happens to a former host after the demon leaves?
- Which quests are fully shared world events, and which are faction-centred situations with an authored aftermath?
- Which abilities are host-specific, equipment-specific, faction-specific, or retained by the demon?
- How should the game clearly communicate a target's possession difficulty and a quest's current world state?

## Related Documents

- [Drain and Possession](../canon/systems/possession.md): established baseline and current in-game interaction.
- [Gameplay Lore Hooks](gameplay-lore-hooks.md): broader narrative support for gameplay systems.
- [Main Story Arc](../canon/story/main-arc.md): the shared long-term story structure.
- [Chapter 0: Prologue](../canon/story/chapters/00-awakening.md): the current opening sequence.
