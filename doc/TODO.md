# TODO: Next Steps for Game Development

## :rocket: High Priority

### Player swimming animations
- **Draw new swimming animations for the player**

### Manage child entities using masks similarly to collision layers
- **Each entity has a childMask that stores which childern the entitie has.**
- **Access the childern using a vector storing the childIDs in the same order as the mask bitset. Thus the correct child can always be found**
- **Update the child mask and ID vector dynamically to stay up-to-date**

### More attacks
- **Different attacks for the rooter and goblin**
  - Goblin uses normal melee attacks
  - Rooter throws sticks until they run out. When empty the stick regenerate
- **Player can pick up a sword that can be swinged for melee damage**  
  - Draw/animate the sword
  - Add the sword to the game
  - Implement the swinging mechanincs using rendering angle

## 🎯 Medium Priority

### Start developing story/progression manager
- **Story manager**
- **Progression manager**

### :heart: Remove HP heart system and use a health bar instead

# :robot: AI suggested improvements

### 1. Improve Player Mechanics
- **Swimming Enhancements**
  - Add buoyancy effect (smooth movement in water)
  - Add water surface movement animation

### 2. Advanced Combat System
- **Melee Attacks**
  - Create attack animation
  - Add enemy hit reactions
- **Ranged Attacks**
  - Implement projectile movement & collision detection
  - Add different weapon types (bow, magic, etc.)
- **Enemy AI**
  - Implement patrolling behavior
  - Add attack logic based on distance
  - Create dodge & retreat behavior
  - A* pathing


### 3. Dynamic World System
- **Chunk Loading System**
  - Implement procedural generation for new areas
  - Improve saving/loading of world state
- **Weather System**
  - Add rain, snow, and fog effects
  - Implement time-based weather changes
  - Adjust player/enemy movement in different weather
- **Day/Night Cycle**
  - Modify lighting based on time
  - Adjust enemy behavior at night
  - Add time-sensitive events (shops close at night, etc.)

### 4. Inventory & UI System
- **Inventory Management**
  - Implement drag-and-drop system
  - Add stackable items (e.g., potions)
  - Display detailed item descriptions
- **Quest System**
  - Implement quest tracking UI
  - Add main and side quests
  - Create a dialog system for NPCs

### 5. Improved Rendering & Visual Effects
- **Layered Rendering System**
  - Optimize sprite batching for performance
- **Special Effects**
  - Implement light sources & shadows
  - Create particle effects for magic & attacks

### 6. AI suggested code improvements
- **1. FPS Counter Rendering (HIGH Impact - 5-10% FPS Gain)**
  - Issue: FPS text is rendered every frame with string concatenation, font lookups, and texture creation/destruction.
  - Location: Game.cpp:130-176
  - Fix: Cache the FPS texture and only update it when the value changes (5-minute effort).
- **2. Scene Rendering Cache Miss (MEDIUM Impact - 8-15% FPS Gain)**
  - Issue: Screen center, zoom, and layer data are recalculated every frame for hundreds of entities.
  - Location: Scene.cpp:30-60
  - Fix: Cache these values and only update when camera/window state changes (30-minute effort).
- **3. ECS Component Pool Lookups (MEDIUM Impact - 10-20% FPS Gain)**
  - Issue: Frequent type_index hash lookups in unordered_map for component access in hot paths.
  - Location: ECS.hpp:180-210
  - Fix: Cache pool pointers or use compile-time component IDs with direct array indexing (1-2 hour effort).
- **4. Level Loading & Quadtree Inefficiencies (MEDIUM-HIGH Impact - 15-30% FPS + Memory Gain)**
  - Issue: Entire pixel matrix loaded at once, shared_ptr overhead in quadtree, expensive vector operations.
  - Location: Level_Loader.cpp:18-50, Quadtree.h:1-80
  - Fix: Implement streaming chunk loading, replace shared_ptr with unique_ptr, optimize vector operations (2-3 hour effort).
- **5. Excessive String/Memory Operations (MEDIUM Impact - 5-8% FPS Gain)**
  - Issue: String concatenations, asset lookups by string, config duplication in hot paths.
  - Location: Assets.h, Scene_Play.h:32-48
  - Fix: Cache hot assets, use compile-time IDs instead of strings, consolidate configs data-driven (2-hour effort).
