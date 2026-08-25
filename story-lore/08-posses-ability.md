# Demon Drain and Possession Ability

## Narrative Baseline

The player is Gudloth, returned as a weak demon or spirit that drains life and possesses other mobs. The mechanic is his means of survival, growth, and access to the four major faction perspectives.

The prologue provides the fixed first example: the demon drains a chicken to regain strength on the ancient battlefield. It then gets past the battlefield guard and reaches the merchant road, where its first major host is a dwarf, elf, wizard, or knight. This design note must not imply that the guard knight is automatically possessed or becomes the first lasting host.

## Two Related Outcomes

The final input design is open. Drain and possession can be two abilities, or one TAKE OVER action with different outcomes based on strength, input, or player choice.

### Drain

Drain is appropriate for a weak target or a target the player chooses not to inhabit.

- The demon begins a vulnerable ritual.
- The target is immobilized while the ritual runs.
- The player may pay HP, time, or another risk while draining.
- On completion, the target dies and the demon gains HP, potentially with a bonus multiplier.
- An interruption can cancel the action and leave the demon exposed.

### Possession

Possession lets the demon take control of a living body.

- The host grants its faction identity, abilities, and temporary social position.
- The demon retains a persistent inventory and carries some progression between hosts.
- The host may also have a body-specific inventory; the transfer rules need to be decided.
- A host can have its own level limit, strengths, and weaknesses.
- Leaving or losing a host must have a clear return-to-demon or next-host rule.

Possession should not erase the host's narrative identity. Friends, family, faction members, and enemies can respond to the body the player is using.

## Suggested Interaction Flow

1. The player targets an eligible mob and presses the TAKE OVER key (Q).
2. A timer, sound, and visual effect begin, such as purple energy flowing from the target to the demon.
3. The target is paralyzed and the player is vulnerable to interruption.
4. At the end of the first stage, the player chooses to finish the drain or commit to possession.
5. Draining kills the target and restores the demon. Possession transfers player control to the target body.
6. UI feedback clearly distinguishes the two outcomes, current host, host inventory, and persistent demon inventory.

A two-stage interaction is optional. A simpler tap-to-drain / hold-to-possess design could communicate the same choice if it remains readable in combat.

## Conditions and Open Design Decisions

- Is eligibility based on CPossessable, current HP, demon power, a target state, or a combination?
- Can the player possess any living mob, only prepared targets, or only specific factions and classes?
- Is possession permanent until the host dies, temporary, or limited by a resource?
- What is lost when the player is interrupted?
- Can a former host be revisited?
- Which memories, social clues, or faction reactions reveal that a body is possessed?
- How are enemy targeting, AI, and allegiances changed immediately after possession?

These decisions should preserve the opening constraint: the demon is too weak to possess the battlefield guard before draining the chicken, while the merchant road provides the first major faction choice.
