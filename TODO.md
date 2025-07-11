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

