# Scene_Play File-Level Refactor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Split the large `Scene_Play.cpp` implementation into responsibility-focused translation units without changing gameplay behavior or `Scene_Play` interfaces.

**Architecture:** Keep `Scene_Play` as the sole owner of gameplay state and preserve its current declarations in `Scene_Play.hpp`. Move cohesive existing member-function definitions into world/persistence, gameplay-system, rendering, and combat translation units; retain construction, input dispatch, frame orchestration, and scene lifecycle in `Scene_Play.cpp`. Register each new source in the existing runtime target.

**Tech Stack:** C++20, CMake, MSYS2 UCRT64/Ninja on Windows, project CTest suites.

**Spec:** Approved scope from the user request: file-level `Scene_Play` refactor; no standalone design spec is required for this behavior-preserving source reorganization.

## Global Constraints

- Keep the `Scene_Play` public and protected method signatures unchanged.
- Keep gameplay method bodies unchanged during extraction; do not combine cleanup or redesign with the moves.
- Preserve update sequencing exactly: `sLoader -> sAI -> sAttack -> sMovement -> sStatus -> sCollision -> sAnimation -> sAudio`; rendering and ECS/renderer updates retain their current placement.
- Do not edit generated/build/runtime output directories.
- Windows C++ validation uses `C:\msys64\ucrt64\bin\cmake.exe`, with `C:\msys64\ucrt64\bin` prepended to `PATH`.

---

## File Structure

- `src/scenes/Scene_Play.cpp` — constructor and constructor-local setup, `sDoAction`, `update`, pause/scene lifecycle, camera/player-ID coordination, and other central orchestration that does not belong to a moved group.
- `src/scenes/Scene_Play_World.cpp` — layout loading, save restore/write, inventory JSON restoration, entity/player/item spawning and world placement bookkeeping.
- `src/scenes/Scene_Play_Systems.cpp` — streaming/terrain notifications, AI, movement, collision, status, animation, and audio frame systems.
- `src/scenes/Scene_Play_Render.cpp` — health/currency/inventory UI and render composition.
- `src/scenes/Scene_Play_Combat.cpp` — attack state, projectiles/hitboxes, swimming/shadows, facing state, line-of-sight/patrol helpers, possession, and inventory handoff helpers closely coupled to possession.
- `CMakeLists.txt` — add the four new translation units to `heavenhell_runtime`.
- `src/scenes/Scene_Play.hpp` — expected to remain unchanged; it continues to declare the class and its existing methods/state.

## Extraction Map

Move existing definitions without rewriting their bodies:

- World/persistence: `loadActiveLayout`, `restoreSavedWorld`, `saveGame`, `findItemFromJson`, `loadInventoryFromJson`, both `updateActiveItem` overloads, `activeItemUseRange`, `useActiveConsumable`, both `addCurrencyToPlayer` overloads, `SpawnFromJSON`, `Spawn`, `DropItem`, and `spawnPlayer`.
- Systems: `sLoader`, `onTerrainChanged`, `sAI`, `sMovement`, `sCollision`, `sStatus`, `sAnimation`, and `sAudio`.
- Rendering: `sRenderHealth`, `sRenderCurrency`, `sRenderInventory`, `sRenderUI`, and `sRender`.
- Combat: `startAttack`, `finishAttack`, `sAttack`, `updateSwimmingState`, `spawnSwimming`, `spawnShadow`, `spawnProjectile`, `destroyProjectile`, `spawnHitbox`, `changePlayerState`, `rayIntersectsAABB`, `hasLineOfSight`, `tickPatrol`, `tryPossess`, and `addItemToInventory`.
- Coordinator: constructor, `sDoAction`, `printHoveredEntityComponents`, `update`, `onEnd`, `onFinish`, `OnPlayerDeath`, `setPaused`, `togglePause`, `getCameraPosition`, and `changePlayerID`.

Keep anonymous-namespace helpers beside their consumers: `placementSignature` and `loadJsonFile` in the world file (the latter is used by `spawnPlayer`); facing/FOV helpers in the systems file. Keep `activeTerrainPath` in the coordinator file because construction uses it. Do not introduce a shared internal header unless extraction proves a helper is used by multiple new translation units.

### Task 1: Extract world and persistence functions

**Files:**
- Create: `src/scenes/Scene_Play_World.cpp`
- Modify: `src/scenes/Scene_Play.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: declarations and member state already present in `Scene_Play.hpp`.
- Produces: out-of-line definitions for the world/persistence methods listed in the extraction map; signatures remain exactly as declared in the header.

- [ ] **Step 1: Create the world translation unit with includes.** Add `Scene_Play.hpp`, `external/json.hpp`, and only the standard/project headers required by the moved definitions. Move `placementSignature` into this file's anonymous namespace.
- [ ] **Step 2: Move the world/persistence method definitions.** Relocate the methods listed in the world/persistence extraction map verbatim from `Scene_Play.cpp`; keep ordering and expressions intact.
- [ ] **Step 3: Remove moved definitions and now-unused includes from the coordinator.** Leave constructor helpers `activeTerrainPath` and `loadJsonFile` in `Scene_Play.cpp`.
- [ ] **Step 4: Register `Scene_Play_World.cpp` in `heavenhell_runtime`.** Add it adjacent to `Scene_Play.cpp` in the source list in `CMakeLists.txt`.
- [ ] **Step 5: Build and test the extraction.** Run from PowerShell:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
& C:\msys64\ucrt64\bin\cmake.exe --build --preset windows-ninja-debug
& C:\msys64\ucrt64\bin\ctest.exe --preset windows-ninja-debug --output-on-failure
```

Expected: build succeeds and all registered tests pass.

### Task 2: Extract gameplay systems

**Files:**
- Create: `src/scenes/Scene_Play_Systems.cpp`
- Modify: `src/scenes/Scene_Play.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: the unchanged `Scene_Play` declarations and current per-frame call sequence.
- Produces: definitions for streaming/terrain notification and AI, movement, collision, status, animation, and audio methods.

- [ ] **Step 1: Create the systems translation unit.** Include `Scene_Play.hpp` and the dependencies used by the moved definitions. Place the existing `PHYSICS_DT`, movement multipliers, `STOP_SPEED`, `FIELD_OF_VIEW_MIN_DOT_PRODUCT`, `facingDirection`, and `isWithinFieldOfView` in its anonymous namespace.
- [ ] **Step 2: Move the listed systems definitions verbatim.** Move `sLoader`, `onTerrainChanged`, `sAI`, `sMovement`, `sCollision`, `sStatus`, `sAnimation`, and `sAudio`. Do not reorder calls in `Scene_Play::update`.
- [ ] **Step 3: Remove moved definitions and system-only includes/constants from `Scene_Play.cpp`.** Preserve its update body unchanged.
- [ ] **Step 4: Register `Scene_Play_Systems.cpp` in `heavenhell_runtime`.**
- [ ] **Step 5: Build and test.** Run the same Windows Ninja debug build and CTest commands from Task 1. Expected: successful build and all tests pass.

### Task 3: Extract rendering

**Files:**
- Create: `src/scenes/Scene_Play_Render.cpp`
- Modify: `src/scenes/Scene_Play.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: existing scene rendering APIs and methods declared in `Scene_Play.hpp`.
- Produces: unchanged implementations of UI rendering and the scene render composition.

- [ ] **Step 1: Create the render translation unit.** Include `Scene_Play.hpp` and the asset/render declarations required by the moved code.
- [ ] **Step 2: Move rendering definitions verbatim.** Relocate `sRenderHealth`, `sRenderCurrency`, `sRenderInventory`, `sRenderUI`, and `sRender`.
- [ ] **Step 3: Remove moved definitions and render-only includes from `Scene_Play.cpp`.** Keep the calls to `sRender()` in `update` at their original position.
- [ ] **Step 4: Register `Scene_Play_Render.cpp` in `heavenhell_runtime`.**
- [ ] **Step 5: Build and test.** Run the Windows Ninja debug build and CTest commands from Task 1. Expected: successful build and all tests pass.

### Task 4: Extract combat and possession

**Files:**
- Create: `src/scenes/Scene_Play_Combat.cpp`
- Modify: `src/scenes/Scene_Play.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: unchanged scene/ECS/component APIs and existing combat method declarations.
- Produces: definitions for attacks, combat entity lifecycles, patrol/line-of-sight, possession, and possession inventory handoff.

- [ ] **Step 1: Create the combat translation unit.** Include `Scene_Play.hpp`, JSON support, and the standard headers required by the moved methods.
- [ ] **Step 2: Move the combat definitions verbatim.** Relocate all methods listed in the combat extraction map, including `addItemToInventory`; keep possession ordering, active-item copying, weapon target-mask adjustment, and player-ID changes exactly as they are.
- [ ] **Step 3: Remove moved definitions and combat-only includes from `Scene_Play.cpp`.**
- [ ] **Step 4: Register `Scene_Play_Combat.cpp` in `heavenhell_runtime`.**
- [ ] **Step 5: Build and test.** Run the Windows Ninja debug build and CTest commands from Task 1. Expected: successful build and all tests pass.

### Task 5: Verify file boundaries and behavior-preserving scope

**Files:**
- Review: `src/scenes/Scene_Play.hpp`
- Review: `src/scenes/Scene_Play.cpp`
- Review: `src/scenes/Scene_Play_World.cpp`
- Review: `src/scenes/Scene_Play_Systems.cpp`
- Review: `src/scenes/Scene_Play_Render.cpp`
- Review: `src/scenes/Scene_Play_Combat.cpp`
- Review: `CMakeLists.txt`

- [ ] **Step 1: Confirm each declared `Scene_Play` method has exactly one definition.** Search all `Scene_Play*.cpp` files for `Scene_Play::` definitions and compare them with declarations in `Scene_Play.hpp`.
- [ ] **Step 2: Confirm the update order and paused-frame behavior.** Check that `Scene_Play::update` has the original systems sequence, render placement, ECS/renderer updates, restart handling, and story-finished handling.
- [ ] **Step 3: Review the diff for implementation changes.** Confirm method bodies were relocated rather than rewritten and that no unrelated files changed.
- [ ] **Step 4: Run final validation.**

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
& C:\msys64\ucrt64\bin\cmake.exe --build --preset windows-ninja-debug
& C:\msys64\ucrt64\bin\ctest.exe --preset windows-ninja-debug --output-on-failure
```

Expected: build succeeds and all registered tests pass.

## Self-Review

- Coverage: all current responsibility groups and the build registration are covered by Tasks 1–5.
- Placeholder scan: no unspecified work items or implementation decisions remain; method groups and validation commands are explicit.
- Interface consistency: all moved functions remain `Scene_Play` members and rely on unchanged declarations; no new interfaces are introduced.
- Scope: this plan intentionally does not add Scene_Play-specific behavioral tests because it changes translation-unit boundaries only. Existing CTest suites and full runtime compilation verify integration; diff review checks behavioral preservation.
