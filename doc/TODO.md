# TODO: Next Steps for Game Development

This file tracks broad game and code-health work. For backend/setup priorities,
see [BackendSetupRoadmap.md](BackendSetupRoadmap.md).

## Code Quality And Backend Status

### Still Worth Doing Before Big Feature Pushes

- [ ] Make render driver selection configurable instead of hard-coding OpenGL
  in `Game.hpp`.
- [ ] Split reusable engine code from Heaven-Hell-specific game code.
- [ ] Add compile warnings to CMake.
- [ ] Improve release packaging so source art files and unused/old assets are
  not copied into player builds unless intentionally wanted.
- [ ] Move audio resource ownership out of `SDLPlatform` into a small audio
  layer or resource system.
- [ ] Move level loading out of `src/physics` into a world/level module.
- [ ] Replace config text parsing in `Scene_Play::loadConfig` with JSON.
- [ ] Cache or throttle SDL text texture creation in `SDLRenderBackend`.

## Gameplay Priorities

### High Priority

#### Player Swimming Animations

- Draw new swimming animations for the player.
- Wire the animation into the movement/state system.

#### Child Entity Ownership

- Track child entities using a child mask or clearer child collection.
- Keep child IDs synchronized when children are created or removed.
- Use the system for shadows, dialogs, projectiles, hitboxes, and other
  parented entities.

#### More Attacks

- Give rooters and goblins different attacks.
- Let goblins use normal melee attacks.
- Let rooters throw sticks that regenerate after they run out.
- Use transform angle/render angle for sword swing placement.

### Medium Priority

#### Story And Progression

- Continue developing the story manager.
- Add a clearer progression manager.
- Add quest tracking UI.

#### Health UI

- Replace the heart HUD with a health bar if that still matches the game
  direction.

## AI Suggested Improvements To Revisit Later

### Player Mechanics

- Improve swimming movement.
- Add water surface movement animation.

### Combat

- Add attack animations.
- Add enemy hit reactions.
- Add more weapon types.
- Expand projectile behavior and collision handling.
- Improve enemy AI with patrol, chase, dodge, retreat, and pathfinding.

### World Systems

- Continue chunk loading improvements.
- Consider procedural generation later.
- Improve saving/loading of world state.
- Consider weather and day/night systems only after the core loop is stable.

### Inventory And UI

- Add drag-and-drop inventory behavior.
- Add stackable items.
- Display detailed item descriptions.
- Improve dialog UI and quest UI.

### Rendering And Effects

- Add light sources and shadows once OpenGL world projection is stable.
- Add particles for magic, attacks, pickups, and damage feedback.
- Consider camera culling before submitting world draw commands.
