# Scene_Play Remaining Review Notes

This document tracks the follow-up items from the `Scene_Play.cpp` review after the high-priority fixes were implemented.

Already handled:

- Entity `0` can now have components removed or replaced.
- Player and enemy weapon use now shares the `CAttackState` path.
- Weapon attacks require `useHeld` through the hit frame.
- The unused projectile `Creating` phase and `beginProjectileFlight` path were removed.
- The `fireball_create` asset entry was intentionally kept for future charge visuals.

## Medium Priority

### Clarify Input Semantics

`CInput::use` is currently the command that starts use, while `CInput::useHeld` is the level state used to keep charging until the hit frame. That is workable, but the names are easy to confuse.

Recommended next step:

- Rename later to something like `usePressed` and `useHeld`, or document the convention directly on `CInput`.
- Keep `use`/`usePressed` as a one-frame command that systems consume.
- Keep `useHeld` as the current button/AI intent state.
- Remove old unused fields such as `shoot` and `canShoot` if no current code path needs them.

### Avoid Resetting Whole `CInput` For NPCs

`sMovement` currently clears NPC input with `inputs = CInput()`. This works today, but it is broad: adding future input fields can accidentally make movement erase action, dialogue, possession, or AI intent state.

Recommended next step:

- Replace whole-component resets with a helper such as `clearTransientAIInput(CInput&)`.
- Clear movement intent and consumed one-frame actions explicitly.
- Leave long-lived state, or intentionally clear it in the system that owns it.

### Reduce `Scene_Play.cpp` System Size

`Scene_Play.cpp` is doing scene orchestration, save/load, spawning, AI, movement, item use, combat, collision calls, rendering, audio, and UI. The systems are readable in isolation, but the file is large enough that behavior dependencies are getting implicit.

Recommended next step:

- Extract item/inventory helpers first: active item lookup, consumable use, inventory load/save.
- Extract combat helpers next: weapon resolution, attack-state timing, hitbox/projectile spawn.
- Keep `Scene_Play` as the high-level system order coordinator until the extracted code stabilizes.

### Document System Order Dependencies

The update order is currently important:

`sAI -> sAttack -> sMovement -> sStatus -> sCollision -> sInteraction -> sAnimation -> sAudio`

Examples:

- AI writes `use` and `useHeld` before `sAttack`.
- `sAttack` reads animation frame data before `sAnimation` advances animations.
- Attack hitboxes are spawned before interaction handling.
- `sMovement` clears NPC inputs after attack handling.

Recommended next step:

- Add a short comment near `Scene_Play::update()` documenting the intentional order.
- When adding combat tests, include at least one test that proves hit-frame timing stays stable.

### Separate Active Item State From Item Copies

`CInventory` stores both `items` and a copied `activeItem`. That makes common reads simple, but it creates a sync rule: whenever a slot changes, `updateActiveItem` must be called or `activeItem` can become stale.

Recommended next step:

- Prefer storing `activeIndex` only and using an accessor to retrieve the active item from `items`.
- If keeping the copy, document that all slot mutation must go through helpers that refresh `activeItem`.

### Make Consumable Use Rules Explicit

`useActiveConsumable` currently returns success at full HP without consuming the item. For the player this is probably fine, but for enemies it can become a repeated no-op if their active item is a potion and they are already full health.

Recommended next step:

- Decide the intended rule: fail at full HP, succeed without consuming, or consume anyway.
- Add future AI logic that only chooses a potion when healing is useful.
- Add a small test or manual checklist item for player and enemy potion behavior.

### Untangle Weapon Runtime Cache

The active item owns weapon config, while `CWeapon` is a runtime component installed when the item becomes active. This is a reasonable bridge, but it creates a few policy questions.

Recommended next step:

- Decide whether cooldown belongs to the entity, the active slot, or the item instance.
- Make item switching behavior explicit: should switching away reset cooldown, preserve cooldown, or have per-item cooldown?
- Consider a smaller `CActiveItem`/`CEquippedItem` component later, with combat resolving weapon data from the active item when use begins.

### Move Fireball Constants Into Data

`spawnProjectile` is still fireball-specific: speed, lifetime, create offset, visual, audio, damage types, and explosion animation are hardcoded.

Recommended next step:

- Move projectile tuning into weapon config when a second projectile weapon appears.
- Keep the current hardcoded path until there is another projectile, because premature generalization would add noise.

## Lower Priority

### Rename ComponentPool Queue Method

`ComponentPool::queueRemoveEntity` actually queues component removal, not entity removal. `ECS::queueRemoveComponent` calls it, so the behavior is correct but the name is misleading.

Recommended next step:

- Rename it to `queueRemoveComponent` internally.
- Keep any public ECS API names unchanged unless there is a larger ECS cleanup pass.

### Improve JSON Validation

Many component constructors and spawn paths trust config shape and use `at()` or map lookups. That is acceptable during fast iteration, but bad config can still throw from deep inside spawning.

Recommended next step:

- Add config validation tests for items, mobs, and entities.
- Validate weapon type strings, masks, required item fields, and inventory references.
- Prefer errors that name the config file and field.

### Inventory UI Assumes Fixed Slot Geometry

`CInventory` now supports variable slot counts, but player inventory rendering still assumes the current inventory sprite and `32px` slot spacing. That is fine for three player slots, but it will not scale automatically if the player inventory size changes.

Recommended next step:

- Either document player inventory as fixed at three slots for now, or make the UI derive its background and slot positions from `inventory.size()`.

### Separate Loot From Death Status

`sStatus` currently handles HP death checks and hardcodes coin spawning for dead non-player entities. This is okay for the current game loop, but it mixes health lifecycle with loot policy.

Recommended next step:

- Add a loot/drop component or mob config field when more death rewards are needed.
- Keep `sStatus` focused on lifecycle and let a loot helper decide what to spawn.

### Add Gameplay-Focused Tests

The current regression coverage is strongest for ECS mechanics. The combat/inventory behavior still depends mostly on manual checks.

Recommended next test targets:

- Releasing `useHeld` before the attack hit frame cancels weapon damage.
- Holding `useHeld` through the hit frame spawns melee hitboxes/projectiles.
- Switching active items refreshes `CWeapon` without stale data.
- A potion heals by its configured amount and clears the active slot only when consumed.
- Enemy active potion behavior matches the intended AI rule.

