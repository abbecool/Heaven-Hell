## Demon Possession Ability: Review and Clarified Notes

### Overview
The demon can interact with other mobs using a possession mechanic. There are two possible outcomes:
- drain the mob for HP
- possess the mob and take control of it

This note separates the two actions for clarity, but the final design may choose either one combined ability with different results or two distinct inputs.

### Option 1: Separate Abilities
- `Drain`: consume a weak mob to gain HP.
- `Possess`: take control of a mob and use its abilities temporarily.

### Option 2: Single Ability with Different Outcome
- Use the same action input, but outcome depends on conditions such as mob strength, player strength, or whether the button is tapped vs held.
- Example: tap to drain, hold to possess.

## Drain
- Works best on weak mobs.
- The demon starts a ritual and may temporarily lose HP equal to the target's HP.
- When the drain completes, the demon restores HP and gains a bonus multiplier (for example 1.5x or 2x the drained amount).
- The mob dies at the end of the process.

## Possess
- The demon takes over a mob and controls it.
- The demon retains its own persistent inventory.
- The possessed mob has its own inventory while under the player’s control.
- Items can transfer between the demon’s inventory and the possessed mob’s inventory.
- Only the demon’s inventory persists after leaving the possessed body.

### Possession Duration
- Possession may be temporary or permanent depending on design.
- Temporary possession is simpler for balance and avoids permanent state management.
- Permanent possession would need a clear reset or a new host selection mechanic.

## Suggested Process for Drain/Possess
1. The player targets a mob and presses the TAKE OVER key (`Q`).
2. A timer starts and a visual effect plays (for example, purple energy from the mob to the demon).
3. The target mob is paralyzed during the process.
4. The player may lose HP while the timer runs, up to the amount of the mob’s HP.
5. When the timer completes, the target mob is reduced to zero HP and becomes immobilized.
6. The mob remains alive but incapacitated for a short window.
7. During that window, the player can press `Q` again to fully possess the mob.
8. If the player does not press `Q` again before the window ends, the mob dies and the demon receives bonus HP from the drain.

## Balance and Conditions
- The ability may require the demon to have higher possession power than the target.
- Possible metrics for comparison:
  - `CPossessable` on each mob
  - current HP of the mob vs current HP of the demon
- Only mobs weaker than the demon should be eligible for possession or drain.

## Implementation Questions
1. Should drain and possess be separate abilities, or should one action produce different results depending on input or conditions?
2. If possession is temporary, how long should the player control the mob?
3. If possession is permanent, how does the player return to demon form or switch hosts safely?
4. Should the initial animation and HP cost always occur, even for weak mobs?
5. What happens if the player is interrupted during the timer? Does the process fail, and is any HP still lost?
6. Should the ability be usable on all mobs, or only specific classes/types?
7. How should inventory management work during possession? Should item transfer be free, or limited by time or inventory slots?
8. Should the player keep the possesed mob’s special abilities automatically, or only while in that body?
9. How does this ability interact with enemy targeting and AI? Does the possessed mob become friendly immediately?
10. Are there audio and visual cues for each stage: start, interruption, success, failure, and full possession?

## General Design Questions
- Does the demon always have a distinct demon form, or is the player always a host in a shared form?
- Should the ability be gated by cooldown, resource cost, or a charge meter?
- Is there a natural way to show the player the difference between a drained mob and a possesed mob in-game?
- Should weak mobs be automatically harvested while stronger mobs require a full possession ritual?
- How should the UI communicate inventory persistence and the difference between the demon’s inventory and the current host’s inventory?
