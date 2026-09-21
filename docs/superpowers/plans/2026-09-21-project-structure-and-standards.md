# Project Structure and C++ Standards Implementation Plan

> **For agentic workers:** Read `AGENTS.md` first. Use the `executing-plans` skill, if available, to implement this document task by task in a future session. Use subagents only when the user or applicable repository instructions authorize delegation. Checkbox steps track implementation, not the preparation of this document. Do not start implementation merely because this plan exists.

**Goal:** Improve correctness, system boundaries, content loading, and development standards through six independently verifiable milestones while preserving existing game behavior.

**Architecture:** Retain C++20, the custom sparse-set ECS, SDL3, the two render backends, and JSON-authored content. Strengthen entity and mutation contracts first, introduce an explicit simulation clock second, then extract headless systems and a shared typed content pipeline. Use focused functions and concrete classes rather than a new general-purpose engine framework.

**Tech stack:** C++20, CMake/CTest, MSYS2 UCRT64 GCC/Ninja on Windows, GCC/Clang on Linux, SDL3/image/mixer/ttf, OpenGL, vendored nlohmann JSON.

**Spec:** The self-contained design specification in sections 1–4 of this file captures the six-item roadmap requested after the project review. Read those sections before any milestone; the preceding chat is not required.

## 1. Scope and milestone map

The six items below refer to the **six rows in the review's recommended order**, not its eight discussion headings.

| Milestone | Deliverable | Depends on | Main acceptance gate |
|---|---|---|---|
| M1 | Initialization fixes and portable world-layout fixture | Baseline inspection | Existing suites pass on Windows; startup failure returns failure |
| M2 | Generational entity handles and explicit structural mutation | M1 | Stale handles and deferred work cannot affect recycled entities |
| M3 | Separate update/render and fixed-step scheduling | M2 | Same simulation outcome for different render-frame partitions |
| M4 | Headless movement and collision boundaries | M2, M3 | Systems are exercised without a scene, window, or renderer |
| M5 | Shared validated definitions for runtime and editor | M2, M4 | Definitions load once; invalid content cannot leave partial spawns |
| M6 | CI, dependency hygiene, and incremental C++ conventions | M1–M5 for final enforcement | Windows/core/sanitizer jobs and focused lint/header checks pass |

M6's CI foundation can be introduced after M1 if useful, but dependency checks and formatting adoption must follow the final file structure. Complete each milestone in a buildable state. Never leave the application uncompilable across handoffs.

### Explicit boundaries

- Preserve content, balance, AI decisions, attack timing at 60 simulation ticks/second, possession behavior, collision masks, render order, and editor file formats.
- Performance optimization is limited to removing per-spawn parsing and avoiding obviously unnecessary allocation. Do not replace the ECS, quadtree, renderer, test harness, or JSON library.
- The full scene-stack redesign, a new event-bus subscription framework, save-game redesign, renderer rewrite, networking, multithreading, and engine-wide namespace migration are outside these six milestones.
- M3 does include deferred scene transitions and explicit overlay scheduling because they are necessary for safe update/render separation. Keep the existing scene map during this work.
- M4 extracts movement, contact detection, and contact response. AI and attack orchestration can remain in `Scene_Play` with narrow calls into extracted systems. A full `WorldSimulation` owner is a later extension, not a prerequisite for useful headless tests.
- M5 keeps the existing JSON schema. Stronger validation must first be checked against all shipped data; intentional schema changes require a separate recorded decision.

## 2. Global constraints and execution rules

- Use **C++20**. `std::expected` and `std::move_only_function` are C++23 and are not available in this plan.
- Windows builds use **MSYS2 UCRT64**. Prepend `C:\msys64\ucrt64\bin` to `PATH` before compiler/CMake/CTest commands.
- Read current repository instructions and source before implementing; line numbers from the review are historical hints. Function and type names are the navigation anchors.
- Preserve unrelated user changes. Never hand-edit generated `build/`, `run/`, or `dist/` files. Normal configure/build/test commands may generate their outputs.
- Do not commit or push without an explicit user request. Each task is a useful checkpoint; it is not authorization to commit.
- Use `apply_patch` for source/document edits when working in OpenCode.
- Register **one CTest entry per suite**, using the existing `TestSupport::TestCase`/`runNamedTest` convention. Tests within a suite are not separate CMake registrations.
- For meaningful behavior changes, first add the described regression test, run it to observe the relevant failure, implement the change, then run the affected suite. Compile failures are appropriate only when introducing a new API; they do not replace behavioral tests.
- Broad C++ build/test checks happen at milestone boundaries. Do not repeatedly run every configuration after a comment or naming-only edit.
- Add no mandatory third-party C++ dependencies. Python 3 is acceptable for development-only scripts in M6; no Python dependency is added to the game executable.
- Update documentation and this file's progress notes when contracts change. Any deviation must state the reason, affected interfaces, tests, and downstream task changes.

### Required behavior invariants

Copy these into regression coverage as the relevant systems become testable:

1. System ordering remains `loader -> AI -> attack -> movement -> status -> collision -> animation -> audio`.
2. AI never controls the current player, including immediately after possession.
3. Possession removes AI control and preserves/retargets the acquired active weapon according to `AGENTS.md`.
4. NPC input is consumed before its reset; attack and movement ordering must remain explicit.
5. Newly spawned attacks become eligible for movement/collision in the same simulation tick when that is current behavior.
6. Editor activity never advances an existing play simulation.
7. Pause renders the frozen world beneath the overlay without repeatedly executing gameplay systems.
8. Entity deletion removes render-layer membership and hierarchy links, including index-zero entities.
9. Static collision changes explicitly invalidate/rebuild the static spatial index.
10. Story events reach `StoryManager` before optional listeners, as in `Scene_Play::Emit`.

## 3. Baseline evidence and verification commands

The review on 2026-09-21 observed:

- `cmake --build --preset windows-ninja-debug` succeeded.
- CTest: seven of eight suites passed. `world_layout_tests` failed in `active_layout_persists` because its fixture concatenates native Windows paths into JSON without escaping backslashes.
- Working tree was clean after the review. Re-check it; do not assume it remains clean.
- Core tests already cover vectors, random arrays, sprite definitions, ECS, render views, story, world layouts, and entity catalogs.

### Windows baseline / milestone gate

Verify the repository/build parent paths exist before commands that create build outputs. Run from the repository root:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
& C:\msys64\ucrt64\bin\cmake.exe --preset windows-ninja-debug
& C:\msys64\ucrt64\bin\cmake.exe --build --preset windows-ninja-debug
& C:\msys64\ucrt64\bin\ctest.exe --preset windows-ninja-debug --output-on-failure
```

Stop dependent commands if the preceding command fails. The build's `sync_assets` target updates runtime content; never patch runtime copies.

For an affected suite, substitute its target/name, for example:

```powershell
& C:\msys64\ucrt64\bin\cmake.exe --build --preset windows-ninja-debug --target ecs_tests
& C:\msys64\ucrt64\bin\ctest.exe --preset windows-ninja-debug -R '^ecs_tests$' --output-on-failure
```

### Linux core and sanitizer gates

```sh
cmake --preset linux-core-debug
cmake --build --preset linux-core-debug
ctest --preset linux-core-debug --output-on-failure

cmake --preset linux-core-sanitize
cmake --build --preset linux-core-sanitize
ctest --preset linux-core-sanitize --output-on-failure
```

Tests for newly extracted systems must link to `heavenhell_core`, not `heavenhell_runtime`. A Linux core build must require no SDL headers or libraries.

### Test example conventions

Code examples below are regression-test bodies or interface contracts, not permission to skip direct includes, namespace declarations, or suite registration. Use `TestSupport::require`; register every new case in the suite's `Tests` array. Floating-point comparisons use a documented epsilon, usually `1e-4f` for short integration tests. New suite paths and target names are stated in each task.

### Checkpoint report template

At each milestone, append an execution note with:

```text
Milestone/task:
Files changed:
Contract changes and compatibility notes:
Commands actually run and results:
Manual checks actually performed:
Remaining blockers / deviations:
Next task:
```

Do not claim a sanitizer or visual check ran on a platform where it was unavailable.

## 4. Target dependency and file map

```text
Application / scenes / editor
        |                 |
        v                 v
Game systems         Content repository + factory
        |                 |
        +-------> ECS <---+
        |                 |
        v                 v
Physics geometry     Runtime component value types

Scenes -> presentation adapters -> RenderBackend / SDLPlatform
```

New files are introduced in their owning milestone, not scaffolded all at once:

| Area | New files | Responsibility |
|---|---|---|
| M2 identity | `src/ecs/EntityID.hpp` | Handle, invalid value, hash, display helper |
| M2 mutation | `src/ecs/CommandBuffer.hpp` | Owned deferred structural operations |
| M2 hierarchy | `src/game/systems/HierarchySystem.hpp/.cpp` | Parent/child policies outside generic ECS |
| M3 time | `src/core/FixedStepClock.hpp/.cpp` | Elapsed-time accumulation and catch-up policy |
| M3 input | `src/core/InputFrame.hpp` | Held state and ordered edge buffering contract |
| M4 movement | `src/game/systems/MovementSystem.hpp/.cpp` | Intent forces and movement integration |
| M4 collision | `src/physics/CollisionWorld.hpp/.cpp`, `src/physics/Contact.hpp` | Spatial index and contact discovery |
| M4 response | `src/game/systems/ContactResponseSystem.hpp/.cpp` | Physical response and game response dispatch |
| M4 effects | `src/game/SimulationEffects.hpp` | Typed requests consumed outside physics |
| M5 definitions | `src/world/EntityDefinition.hpp`, `src/world/DefinitionRepository.hpp/.cpp` | Immutable typed content and lookup |
| M5 parsing | `src/serialization/DefinitionParser.hpp/.cpp`, `src/serialization/ComponentJson.hpp/.cpp` | JSON-to-value conversion and diagnostics |
| M5 spawning | `src/world/EntityFactory.hpp/.cpp` | Validated definition instantiation and rollback |
| M5 items | `src/game/items/Item.hpp`, `src/game/items/WeaponConfig.hpp` | JSON-free item/weapon value types |
| M6 standards | `.clang-format`, `.clang-tidy`, `.github/workflows/ci.yml`, `docs/CppStandards.md` | Automated and documented conventions |
| M6 tooling | `scripts/check-format.py`, `scripts/check-dependencies.py` | Scoped formatting and dependency checks |

Existing `Scene`, `Scene_Play`, `Game`, `ECS`, `ComponentPool`, renderers, and content files are migrated incrementally. Keep compatibility wrappers only during a milestone; list and remove them before its acceptance gate.

---

## M1. Establish a reliable baseline

### Task 1.1 — Serialize the world-layout fixture correctly

**Files:** Modify `tests/world/test_world_layout.cpp`; retain `world_layout_tests` in `CMakeLists.txt`.

**Contract:** Fixture construction must produce valid JSON for native Windows paths, quotes, backslashes, and non-ASCII characters. No runtime parser behavior changes.

- [ ] Run the existing world-layout suite and capture the actual failure before editing.
- [ ] Replace `writeRegistry`'s string concatenation with `nlohmann::json` construction and `.dump(2)`. Include `external/json.hpp` directly.

```cpp
const nlohmann::json registry = {
    {"version", 1},
    {"activeLayoutId", active},
    {"layouts", nlohmann::json::array({
        {{"id", "first"}, {"displayName", "First"},
         {"terrainPath", "terrain.png"},
         {"placementPath", (workspace.path / "layouts/first.json").string()}},
        {{"id", "second"}, {"displayName", "Second"},
         {"terrainPath", "terrain.png"},
         {"placementPath", (workspace.path / "layouts/second.json").string()}}
    })}
};
writeFile(workspace.path / "levels.json", registry.dump(2));
```

- [ ] Keep the existing persistence/save/reload/rejection tests; they already provide meaningful coverage of this fix. Do not add a test that merely proves the JSON library can escape strings.
- [ ] Run `world_layout_tests` and all existing suites on Windows. Record remaining failures separately instead of attributing all failures to the fixture.

**Acceptance:** All four internal world-layout cases run, including `default_layout_migration`; no fixture string manually embeds a filesystem path into JSON.

### Task 1.2 — Initialize objects and propagate startup failure

**Files:** Modify `src/core/Game.hpp`, `src/core/Game.cpp`, `src/main.cpp`, `src/ecs/Entity.hpp`, `src/ecs/Components.hpp`; inspect `src/core/SDLPlatform.hpp/.cpp` for ownership declarations.

**Interfaces:** `Game` construction either returns a usable object or throws. `main` logs one contextual startup error and returns `EXIT_FAILURE`. Default `Entity` is explicitly invalid until M2 introduces a generational handle.

- [ ] Audit scalar members in these classes and all `C*` default constructors. Set `m_currentFrame = 0`, `m_paused = false`, initialize default entity pointer to `nullptr`, and use one named invalid integer ID until M2 replaces it. Initialize `CParent::parent` to that invalid value. Do not treat zero as invalid: it is an existing valid entity ID.
- [ ] Make methods on an invalid default `Entity` report a programming error rather than dereference an indeterminate pointer. Queries can return false; component access/mutation throws `std::logic_error`. M2 will consolidate this with liveness checking.
- [ ] Remove constructor catches that set `m_running = false` and return after platform/backend failure. Let startup exceptions propagate through RAII-owned members.
- [ ] Handle application-boundary failure in `main`:

```cpp
int main(int, char*[]) {
    try {
        Game game("config_files/assets.json");
        game.run();
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Heaven-Hell failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
```

- [ ] Replace FPS nanosecond accumulation stored in `int` with `std::chrono::nanoseconds` or `std::int64_t`; guard division by zero and retain fractional precision until display formatting. M3 will replace frame scheduling, not depend on this diagnostic counter.
- [ ] Verify resource destruction order: scenes first, asset metadata next, renderer next, SDL platform last. Remove manual early backend/platform resets in `Game::run()` if they destroy dependencies while scenes still exist. Audit constructors that acquire raw SDL/GL resources and throw: use scoped guards/custom-deleter owners where the owning object's destructor would not run on constructor failure.
- [ ] Add an ECS-suite case for invalid default `Entity` queries/access. Build/run once for a failure before the change and once after.
- [ ] Build the full game. Launch normally and quit. Launch with the working directory set to an approved empty temporary directory so required content cannot load; verify nonzero exit and a useful error. Do not rename/delete source assets to simulate failure.

**Acceptance:** No uninitialized scalar remains in the audited default states; startup failure is observable through process status; normal shutdown preserves resource lifetime ordering. Run the Windows milestone gate.

---

## M2. Define entity validity and ECS mutation rules

### Task 2.1 — Introduce generational entity identity

**Files:** Create `src/ecs/EntityID.hpp`; modify `src/ecs/ECS.hpp`, `src/ecs/ComponentPool.hpp`, `src/ecs/Entity.hpp`, `src/ecs/Components.hpp`, `src/physics/Renderer.hpp`, `src/physics/Level_Loader.hpp`, `src/story/EventBus.hpp`, `src/debug/EntityInspector.hpp/.cpp`, and all entity-ID consumers under `src/` and `tests/ecs/`.

**Public contract:** Retain the spelling `EntityID` to reduce migration noise, but make it a strong handle. Internal slot indices are `std::uint32_t`, never handles.

```cpp
struct EntityID {
    std::uint32_t index = UINT32_MAX;
    std::uint32_t generation = 0;
    auto operator<=>(const EntityID&) const = default;
};
inline constexpr EntityID InvalidEntity{};
```

Use `std::numeric_limits<std::uint32_t>::max()` in the actual header rather than relying on a macro include. Provide `EntityIDHash` and a `std::hash<EntityID>` specialization delegating to it so existing unordered containers migrate consistently. Hash both fields. Provide `std::string entityIdString(EntityID)` for diagnostics, formatted `index:generation`; do not add implicit integer conversions.

- [ ] Add lifecycle tests that explicitly distinguish a recycled slot from its old handle:

```cpp
ECS ecs;
const EntityID old = ecs.addEntity();
ecs.removeEntity(old);
const EntityID replacement = ecs.addEntity();
require(old.index == replacement.index, "slot was not recycled");
require(old.generation != replacement.generation, "generation did not advance");
require(!ecs.isAlive(old), "stale handle became live");
require(ecs.isAlive(replacement), "replacement not live");
```

- [ ] Maintain generation and sparse-index arrays by slot. Free-list entries are indices. Dense entity storage contains full handles. `isAlive` checks bounds, generation, and dense membership. Increment the generation on destruction. Retire a slot at generation exhaustion instead of wrapping and accepting ancient handles; test this through a small allocator-level test seam, not billions of allocations.
- [ ] Index component sparse arrays by `id.index`; verify `denseEntities[sparse[id.index]] == id` on lookup. Reject insertion for a different generation already occupying a pool slot. Only ECS allocates identities; standalone pool tests use explicit `EntityID{index, generation}` values.
- [ ] Validate liveness at ECS public operations. `hasComponent`/`tryGet` return false/null for invalid or stale handles. `getComponent`/`addComponent`/copy/move throw on invalid targets or sources. Removal of a stale handle is an idempotent no-op. Deferred stale commands must never affect a replacement entity.
- [ ] Update tests that currently compare IDs directly with `0`, `1`, or `INITIAL_ENTITY_CAPACITY` to compare `.index` where slot order is the subject. Replace `return -1`, `EntityID = 0`, and integer sentinels in spawning, parent links, projectile owners, scene player state, and inspector/renderer caches with `InvalidEntity`.
- [ ] Update hashing, ordering, printed IDs, collision deduplication keys, and unordered containers. A hash value is not an identity: replace trigger deduplication's `unordered_set<uint64_t>` of pre-hashed entity pairs with a struct containing both full handles and both layer indices, equality, and a hash functor. Keep shape-index pair keys separate from entity-handle keys.
- [ ] Audit `EntityID` declarations using repository search; remove every repeated alias and the nested renderer alias. Include the new header directly.
- [ ] Keep handles local to one ECS instance. `Entity` stores its owning ECS pointer plus handle; passing a raw handle from another world is an API precondition violation, not something generations alone detect. Document this restriction.
- [ ] Build all targets and run the ECS and render suites. Run sanitizers on Linux when available.

**Acceptance:** An old parent/projectile/selection/deferred handle cannot address a new occupant of its former slot. No implicit handle-to-integer conversion exists.

### Task 2.2 — Make lookup and snapshot semantics safe

**Files:** Modify `src/ecs/ECS.hpp`, `src/ecs/ComponentPool.hpp`, `src/ecs/Entity.hpp`, `src/ecs/View.md`, `tests/ecs/test_ecs.cpp`.

**Interfaces:**

```cpp
template<class T> T* tryGet(EntityID id);
template<class T> const T* tryGet(EntityID id) const;
template<class T> bool hasComponent(EntityID id) const;
template<class... Ts> std::vector<EntityID> snapshotEntities() const;
```

- [ ] Add tests for missing lookup in two independent worlds, stale lookup, and nested snapshots:

```cpp
const auto first = ecs.snapshotEntities<CTransform>();
const auto second = ecs.snapshotEntities<CVelocity>();
require(first == expectedTransforms, "nested query overwrote snapshot");
require(second == expectedVelocities, "velocity snapshot incorrect");
```

`expectedTransforms` and `expectedVelocities` are owned vectors populated from the IDs created in that case; use disjoint entity sets so accidental reuse cannot pass.

- [ ] Make all presence checks non-allocating by routing them through `findComponentPool`. Remove the non-const presence implementation that creates pools.
- [ ] Eliminate the function-static mutable empty pool. Missing `getComponentPool<T>()` throws; internal nullable lookup is separate. Remove the unreachable diagnostic after its old `return`.
- [ ] Use checked type-index invariants and `static_cast` for pool downcasts instead of `reinterpret_cast`.
- [ ] Replace `ViewEntities` calls with owned `snapshotEntities` results and remove the ECS scratch vector. Document that snapshots preserve IDs, not component references or liveness.
- [ ] Make dense/sparse bookkeeping inaccessible to callers. Keep read-only entity spans/ranges for views; no public mutable `denseEntities`, removal queues, or mutable dense vector accessor.
- [ ] Run the ECS suite and compile the application to discover downstream API assumptions.

**Acceptance:** Querying never changes world storage; nested queries cannot overwrite prior results; missing pools cannot leak state between worlds.

### Task 2.3 — Separate hierarchy policy and implement owned structural commands

**Files:** Create `src/ecs/CommandBuffer.hpp`, `src/game/systems/HierarchySystem.hpp/.cpp`; modify ECS/hierarchy callers, `src/scenes/Scene.cpp`, `src/scenes/Scene_Play.cpp`, `src/physics/Level_Loader.cpp`, `tests/ecs/test_ecs.cpp`, `CMakeLists.txt`.

**Interfaces:**

```cpp
class CommandBuffer {
public:
    template<class T> void set(EntityID id, T value);
    template<class T> void remove(EntityID id);
    void destroy(EntityID id);
    template<class... Ts> void spawn(Ts... initialComponents);
    void flush(ECS& ecs);
    bool empty() const noexcept;
};

class HierarchySystem {
public:
    static void attach(ECS&, EntityID parent, EntityID child,
                       Vec2 relativePosition, bool removeOnDeath = true);
    static void detach(ECS&, EntityID child);
    static void queueDestroy(ECS&, CommandBuffer&, EntityID root);
    static void updateTransforms(ECS&);
};
```

`spawn` has no immediate entity return: it is for births whose ID is not needed until after the barrier. When a producer must retain a new ID, collect typed spawn requests, instantiate them after its iteration, and reacquire needed components by existing handle. Do not invent callbacks capturing references into component pools.

- [ ] Add command tests: set/remove becomes visible only after flush; FIFO set-then-remove and remove-then-set; stale target ignored; destroy-then-set does not resurrect; spawn is invisible before flush and fully initialized afterward; two queued destroys notify removal observers only once.
- [ ] Implement owned commands using a C++20 type-erased base plus `unique_ptr` command models. Arguments are decayed owned values. Prohibit borrowed component references in commands. Flush moves the pending batch into local storage so newly queued commands cannot invalidate iteration; reject recursive flush. Use one documented batch per flush.
- [ ] `spawn` allocates at flush, adds owned component values, and rolls back the new entity if construction fails. `set` must not destroy an existing component before copying a value that aliases it; materialize a value first. Document that arbitrary out-of-memory failures stop the operation and are not recoverable gameplay events.
- [ ] Move `attachChild`/`detachChild` and cascading destruction policy out of generic ECS. `attach` rejects self-parenting and ancestor cycles before changing links. `queueDestroy` takes an owned traversal snapshot, detaches surviving children, and queues descendants marked `removeOnDeath` once. Raw ECS removal is storage-only; all game-world deletion paths use the hierarchy policy.
- [ ] Add tests for cycle rejection, reparenting, deleting index-zero parent, mixed persistent/deleted children, and stale child links.
- [ ] Retain the entity-removal observer used by renderer cleanup, but document that it must not structurally re-enter ECS. Notify once with the old full handle. Clean renderer membership before the slot can be reused.
- [ ] Check `ComponentPool::addComponent` exception consistency: update sparse bookkeeping only after dense component/ID insertion succeeds; if the second vector insertion throws, undo the first. Constrain supported component move/swap requirements or implement the appropriate rollback so removal preserves both arrays.
- [ ] Link the hierarchy implementation into `heavenhell_core`; remove `Components.hpp` from generic ECS/ComponentPool once their remaining game-specific debug helpers move into `EntityInspector`.

**Acceptance:** Storage does not know game hierarchy components. Queued values own their lifetime. Cycles and stale commands cannot corrupt live entities.

### Task 2.4 — Migrate mutation-heavy systems without dangling references

**Files:** Modify `src/scenes/Scene_Play.hpp/.cpp`, `src/physics/CollisionManager.cpp`, `src/scenes/Scene.cpp`, `src/ecs/View.md`, `tests/ecs/test_ecs.cpp`.

**Scheduling contract:** Component references are valid only until a structural change to their pool. No active system iteration may structurally mutate storage directly. Flushes occur only after all local view/reference variables from that phase are gone.

- [ ] Audit `sAttack`, `startAttack`, `finishAttack`, `spawnProjectile`, `spawnHitbox`, `sStatus`, `tryPossess`, `changePlayerID`, and hierarchy/streaming births. In particular, `sAttack` currently holds inventory/transform/attack references while spawning; copy spawn inputs and queue/collect requests before leaving the loop.
- [ ] Use two kinds of deferred work: **phase commands** for additions/replacements/removals that historically become visible in that tick, and **end-of-tick commands** for operations historically using queued removal. Keep the existing delayed-removal visibility unless a dedicated regression establishes a deliberate correction.
- [ ] Establish named barriers: after loader, after AI, after attack, after movement, after status, after collision, after animation, after audio. End-of-tick removals flush once after audio. M3 moves all flushing out of rendering. Never add a flush in the middle of a view loop.
- [ ] Preserve same-tick attack spawn participation by applying attack birth requests after attack and before movement. Preserve status's immediate collider removal before collision via the status barrier. Preserve originally queued hitbox-component removal timing with the end-of-tick queue; do not silently advance it to the status barrier.
- [ ] Add a stress regression constructing enough transforms/attack entities to force vector growth, then process attack birth requests and verify original attacker state/ownership. Run with ASan/UBSan on Linux.
- [ ] Add debug structural-mutation detection: views capture a structural version and check it on iterator increment/dereference. This catches invalid mutation during iteration; document that it is not a general detector of references retained after a view is destroyed.
- [ ] Remove old component-pool removal queues and route compatibility `queueRemove*` wrappers through the designated command buffer, then remove wrappers when all callers are migrated.

**M2 acceptance:** Full build/suites pass, lifetime contracts are in `src/ecs/View.md`, all game deletion paths preserve hierarchy/render cleanup, and mutation-heavy systems run under sanitizers without use-after-reallocation.

---

## M3. Separate rendering from fixed-step simulation

### Task 3.1 — Add a pure fixed-step scheduler

**Files:** Create `src/core/FixedStepClock.hpp/.cpp`, `tests/core/test_fixed_step_clock.cpp`; modify `CMakeLists.txt` to add them to core and register `fixed_step_clock_tests`.

**Interface:**

```cpp
struct StepBatch {
    unsigned steps = 0;
    double alpha = 0.0;
    double droppedSeconds = 0.0;
};
class FixedStepClock {
public:
    static constexpr double StepSeconds = 1.0 / 60.0;
    static constexpr unsigned MaxStepsPerFrame = 8;
    StepBatch advance(std::chrono::duration<double> elapsed);
    void reset() noexcept;
private:
    double accumulatorSeconds = 0.0;
};
```

Policy: reject nonfinite or negative elapsed time with `std::invalid_argument`; clamp a single measured interval to 250 ms; execute at most eight ticks; discard excess whole ticks after the cap while retaining the fractional remainder. Include both clamp and catch-up discards in `droppedSeconds`. Maintain `0 <= alpha < 1`; test floating-point boundary behavior rather than relying on truncation luck.

- [ ] Add exact/partitioned-time tests, half-tick tests, zero time, 250 ms stall, ten-second stall, invalid input, and reset:

```cpp
FixedStepClock clock;
unsigned total = 0;
for (int i = 0; i < 120; ++i) {
    total += clock.advance(std::chrono::duration<double>(1.0 / 120.0)).steps;
}
require(total == 60, "one second did not produce sixty ticks");
const auto idle = clock.advance(std::chrono::duration<double>(0.0));
require(idle.alpha >= 0.0 && idle.alpha < 1.0, "invalid interpolation fraction");
```

- [ ] Implement the arithmetic without SDL, sleeping, rendering, or global clocks. Wall-clock sampling belongs in `Game`.
- [ ] Run the new suite independently and through core-only CMake.

### Task 3.2 — Split all scene lifecycles and defer transitions

**Files:** Modify `src/scenes/Scene.hpp/.cpp`, all seven concrete `Scene_*.hpp/.cpp` pairs, `src/core/Game.hpp/.cpp`, `src/core/SDLPlatform.cpp`, `src/physics/Camera.hpp/.cpp`.

**Scene API:**

```cpp
virtual void fixedUpdate(float dt) = 0;
virtual void render(float alpha) = 0;
```

Input dispatch remains `doAction`/`sDoAction`; M3.3 specifies buffering. Frame/tick-dependent animation and camera pan state advance in fixed updates, not draw methods.

- [ ] Read each old scene `update()` and list its mutations versus drawing calls. Move mutations to `fixedUpdate`, draw calls to `render`; add `override` on every override. Menus/editor can use the same 60 Hz update cadence during this migration.
- [ ] Change `Scene_Play` so `sRender` never calls simulation/command flushing. Flush M2 end-of-tick work and renderer membership cleanup before rendering. Preserve animation/attack ordering, including attack's use of animation frame state.
- [ ] Keep the existing map but add a concrete `SceneSchedule` value containing a retained `shared_ptr<Scene>` for the input target and vectors of retained scenes for update/render order. Build it once per outer frame; do not repeatedly look up mutable scene names while dispatching.
- [ ] Define schedules: PLAY updates/renders itself; SETTINGS over PLAY updates its UI, renders PLAY then SETTINGS, and blocks play simulation; editor updates/renders only itself. Camera-triggered world pause still permits camera progression in the play scene but skips its gameplay systems.
- [ ] Convert `changeScene` during event/update callbacks into pending transitions, applied at the start/end of an outer frame after callbacks release references. Once a transition is requested, stop additional catch-up ticks for the old scene; reset accumulator on applying a transition so elapsed time from one scene is not spent in another.
- [ ] `currentScene` uses checked lookup rather than `operator[]`; invalid names are programmer errors. Resume with a null scene pointer is permitted only for an existing stored scene. Remove external mutable-map writes after auditing menu/pause/inventory callers; expose specific lookup/request functions instead.
- [ ] Create `src/core/SceneSchedule.hpp/.cpp` for scheduling/transition policy so tests need no `Game` construction; keep it a concrete policy, not a second scene framework. Add `tests/core/test_scene_schedule.cpp` as runtime-linked `scene_schedule_tests`, conditional on `HEAVENHELL_BUILD_GAME`, using fake `Scene` subclasses with counters and no initialized SDL objects. Register it separately from the core-only test helper so runtime linkage is explicit. Test overlay order, no gameplay update beneath SETTINGS/editor, queued self-replacement, and missing-scene rejection.
- [ ] Build all concrete scenes in the same task. Do not leave old `update()` aliases that let simulation run from a draw path.

**Acceptance:** Drawing twice changes no gameplay state. A callback cannot destroy the scene whose method is still running. Paused/editor schedules explicitly express what updates and renders.

### Task 3.3 — Buffer input edges and integrate the outer loop

**Files:** Create `src/core/InputFrame.hpp`, `tests/core/test_input_frame.cpp`; modify `src/core/Game.cpp`, `src/core/SDLPlatform.hpp/.cpp`, `src/scenes/Scene_Play.cpp`, `CMakeLists.txt`; register `input_frame_tests` in core.

**Contract:** Platform polling precedes simulation. UI/window/quit actions are processed once per outer frame. Simulation input retains ordered START/END edges until the next fixed tick, while held state persists across ticks. Do not replay a press edge during every catch-up tick.

Use an owned `InputFrame` with ordered actions, latest mouse position, accumulated wheel delta, and simulation-held action names. Expose this exact contract:

```cpp
struct TickInput {
    std::vector<Action> actions;
    Vec2 mousePosition{};
    int wheelDelta = 0;
    std::unordered_set<std::string> heldActions;
};
class InputFrame {
public:
    void queue(Action action);
    void setMousePosition(Vec2 position);
    void addWheelDelta(int delta);
    TickInput consume();
    void clear();
};
```

`consume` applies ordered START/END edges to the held set, returns an owned snapshot, and clears only pending edges/wheel. It runs once per fixed tick: later catch-up ticks receive empty edges and unchanged held state. Do not update simulation-held state directly at platform poll time, which would collapse a press/release pair before consumption. `clear` clears pending edges, wheel, and held actions; the scene also resets its player `CInput` on focus loss/transition. Include dependencies directly. Use the existing action mapping and action strings during this milestone. Move `src/core/Action.cpp` from `heavenhell_runtime` to `heavenhell_core` in CMake so the headless input suite can link its existing non-inline methods.

- [ ] Add tests: two 120 Hz frames yield one delivery at the next tick; a press/release pair before a tick retains order and still records a press intent for that tick; three catch-up ticks receive one press edge; held movement remains active until release; focus loss/transition clears state; wheel events accumulate before consumption. Scene translation must latch one-shot press intent for the tick independently from final held state, rather than letting an END edge erase an unconsumed START action.
- [ ] Update platform input translation to route gameplay edges through the buffer. Separate window/quit/fullscreen/UI controls from simulation commands explicitly; never delay a close event until enough time for a tick accumulates.
- [ ] Implement the outer-frame sequence:

```text
sample steady-clock elapsed time
apply pending scene transition, resetting clock/input when needed
poll platform events and deliver frame-level UI/window actions
select retained scene schedule
advance FixedStepClock
for each scheduled tick:
    consume input snapshot (edges only on first tick, held state on every tick)
    fixedUpdate(1/60) scheduled scenes
    stop catch-up if transition requested
beginFrame
render(alpha) scheduled scenes, bottom to top
draw diagnostics
endFrame
optional frame pacing using a precise steady-clock deadline
apply pending transition when callbacks/rendering have finished
```

- [ ] Replace `FrametimeHandler`'s integer-millisecond frame cap. Sleeping occurs after presentation if an explicit cap is enabled, uses a duration derived from `1.0 / rate`, and never determines simulation dt. Detect/document backend vsync policy to avoid adding a second unintended wait.
- [ ] First render current simulation positions using the provided alpha without interpolation. Keep `prevPos` unchanged for collision history. Interpolation is optional later and must never overwrite simulation transforms; this milestone's correctness does not require visual interpolation.
- [ ] Keep existing frame-based durations as **60 Hz simulation ticks**, documenting that convention. Make the old `Physics::knockback` timing explicit: pass `dt`, accumulate milliseconds with floating precision, and preserve the nominal configured duration. Characterize its old stepping before changing the hard-coded 16 ms approximation; record any correction in release notes rather than hiding it in extraction.
- [ ] Run clock/input/scene scheduling suites and full Windows build/tests. Manually check play at fast/slow render rates, held movement, tap actions, pause/resume, editor entry/exit, camera pan, and quit. Verify animation/attack speed does not scale with rendering FPS.

**M3 acceptance:** Rendering and polling are independent of tick count; input edges are neither dropped on zero-tick frames nor repeated on catch-up frames; pauses produce no backlog of gameplay ticks.

---

## M4. Extract headless movement and collision boundaries

### Task 4.1 — Extract movement without changing its equations

**Files:** Create `src/game/systems/MovementSystem.hpp/.cpp`, `tests/systems/test_movement.cpp`; modify `src/scenes/Scene_Play.cpp`, `CMakeLists.txt`; register `movement_tests` linked to core.

**Interface:**

```cpp
struct MovementContext {
    EntityID player = InvalidEntity;
    bool playerHealthCritical = false;
};
void updateMovement(ECS& ecs, const MovementContext& context, float dt);
```

- [ ] Move `sMovement`'s force, damping, maximum-speed, stop-threshold, velocity, and transform logic into this function. Preserve its current ordering, including NPC reset and active-attack suppression; do not fix unrelated movement behavior during extraction.
- [ ] Call `HierarchySystem::updateTransforms` after movement. Skip stale parents safely; do not dereference a transform on a destroyed parent. Missing-parent cleanup belongs to hierarchy maintenance, not renderer code.
- [ ] Write fixtures creating `CTransform`, `CVelocity`, `CPhysicsBody`, and `CInput` directly. Test zero-force damping, nonzero-force integration, speed cap, zero damping, previous-position update, player/NPC intent reset, player-critical modifier, and attack-suppressed NPC movement.

```cpp
ECS ecs;
const EntityID id = ecs.addEntity();
ecs.addComponent<CTransform>(id, Vec2{0, 0});
ecs.addComponent<CVelocity>(id, Vec2{60, 0});
ecs.addComponent<CPhysicsBody>(id, 1.0f, 0.0f, 100.0f, 0.0f);
updateMovement(ecs, MovementContext{id, false}, 1.0f / 60.0f);
require(std::abs(ecs.getComponent<CTransform>(id).pos.x - 1.0f) < 1e-4f,
        "kinematic displacement changed");
```

- [ ] Add an integration case using `FixedStepClock` to drive identical worlds with 30 Hz, 60 Hz, and 120 Hz frame partitions for equal simulated time. Compare positions, velocities, and number of ticks; exclude catch-up-cap scenarios from equivalence assertions.
- [ ] Replace `Scene_Play::sMovement` with a narrow call or remove the wrapper. No `Game`, `Scene`, SDL, or render headers may appear in the extracted files.

### Task 4.2 — Separate contact discovery from response

**Files:** Create `src/physics/Contact.hpp`, `src/physics/CollisionWorld.hpp/.cpp`, `tests/physics/test_collision_world.cpp`; modify `src/physics/CollisionManager.hpp/.cpp`, `src/physics/Quadtree.hpp/.cpp`, `CMakeLists.txt`; register `collision_world_tests` in core.

**Interfaces:**

```cpp
struct Contact {
    EntityID first = InvalidEntity;
    EntityID second = InvalidEntity;
    std::size_t firstShape = 0;
    std::size_t secondShape = 0;
    CollisionMask firstLayer{};
    CollisionMask secondLayer{};
    Vec2 separationForFirst{};
    bool trigger = false;
};
class CollisionWorld {
public:
    void setWorldBounds(Vec2 center, Vec2 size);
    void invalidateStaticGeometry() noexcept;
    const std::vector<Contact>& detect(const ECS& ecs);
    const std::vector<ColliderProxy>& proxies() const noexcept;
};
```

Move `ColliderProxy` into `Contact.hpp` or a directly included `ColliderProxy.hpp`; it retains existing fields and full handles. `detect` returns owned internal storage valid until the next detect/bounds change. It must not mutate ECS, dispatch game events, play audio, or draw.

- [ ] Characterize current contact rules before extraction: strict AABB overlap, either-side target-mask acceptance, same-entity exclusion, static/static exclusion, trigger deduplication per entity/layer pair, solid contacts per shape pair, and layer-normalized orientation.
- [ ] Transfer quadtree/static-cache/proxy construction and overlap calculations unchanged. A static-cache invalidation flag triggers rebuilding on next detect. Validate cached full handles against ECS so stale geometry cannot hit recycled entities; invalidate after static transform/collider edits and chunk unloading.
- [ ] Build the whole contact list from the start-of-collision snapshot before applying responses. Preserve current traversal/contact ordering initially because response order can matter. Do not sort as an unrelated cleanup.
- [ ] Keep full structural pair keys from M2; separate equality from hashing. Normalize first/second by collision layer, then a stable full-handle tie-breaker, and invert separation on swaps. Document sign: applying positive `separationForFirst` moves the first body away from the second.
- [ ] Add tests for no overlap, edge touching, masks, offsets/multiple shapes, repeated leaf membership, static/static, invalid bounds, stale static entity, trigger deduplication, and separation orientation. Fixture helpers directly populate `CCollider::shapes` with shape size/half-size/layer fields rather than parsing JSON.
- [ ] Move quadtree debug drawing into presentation code, e.g. `src/render/CollisionDebugDraw.hpp/.cpp`, or expose boundary data to the existing renderer helper. Core geometry must not require `RenderBackend` linkage just to detect contacts.

**Acceptance:** Calling `detect` twice without simulation changes produces equivalent contacts and leaves every component unchanged. The suite builds and runs with `HEAVENHELL_BUILD_GAME=OFF`.

### Task 4.3 — Extract response and expose explicit scene-level requests

**Files:** Create `src/game/systems/ContactResponseSystem.hpp/.cpp`, `src/game/SimulationEffects.hpp`, `tests/systems/test_contact_response.cpp`; modify `src/scenes/Scene_Play.hpp/.cpp`, legacy collision files, `CMakeLists.txt`; register `contact_response_tests` in core.

**Interfaces:**

```cpp
struct PossessRequest { EntityID player; EntityID target; };
struct PickupRequest { EntityID player; EntityID loot; };
struct DialogueRequest { EntityID player; EntityID speaker; };
struct AreaRequest { EntityID player; EntityID area; };
struct ProjectileImpactRequest { EntityID projectile; };
using ContactRequest = std::variant<PossessRequest, PickupRequest,
    DialogueRequest, AreaRequest, ProjectileImpactRequest>;
struct SimulationEffects {
    std::vector<ContactRequest> contacts;
    std::unordered_set<EntityID> entitiesInWater;
};
void resolveBodyContacts(ECS&, std::span<const Contact>);
SimulationEffects collectContactResponses(ECS&, std::span<const Contact>,
                                         CommandBuffer& phaseCommands);
```

These are concrete value requests, not a generic event bus. They carry handles, not `Entity` wrappers or references. `entitiesInWater` is a complete set for the current collision tick, including the empty set; the scene calls existing swimming-state reconciliation once each tick.

- [ ] Move inverse-mass position correction and inward-velocity removal into `resolveBodyContacts`. Preserve immovable-body behavior and test equal/unequal masses and normal velocity removal.
- [ ] Move pure damage/contact eligibility into `collectContactResponses`. Damage values can update existing health in place; new damage-flash components go through phase commands. Preserve attack hit-set behavior and target masks.
- [ ] For projectile impacts, suppress additional impacts for that projectile during the same contact batch before the later scene adapter runs. Do not let queuing destruction turn one projectile hit into multiple damage applications. Use a local full-handle set or update an existing projectile phase in place.
- [ ] Keep inventory, possession, dialogue display, area/story callbacks, and visual projectile teardown in `Scene_Play` initially. It consumes typed requests in contact order after contact iteration has finished and reacquires components by validated handles for every request. Drop requests for entities no longer eligible/alive. Do not replace a scene pointer with an equally broad `GameServices` locator.
- [ ] Apply body correction and game response in the characterized order. If splitting into two passes changes an existing contact outcome, instead process each contact through the two narrow response functions in original sequence; capture the decision with a regression before proceeding.
- [ ] Add headless tests: projectile owner immunity, one impact per tick, damage hit-set suppression, water-set emptiness on exit, friendly/enemy possession requests, loot/dialogue/area request routing, and stale handles. Assertions inspect effects and ECS values; no scene or renderer is constructed.
- [ ] Replace `Scene_Play::sCollision` with detect -> response -> request consumption -> phase flush -> water reconciliation. Remove `Scene_Play*` from all physics classes. Remove the old `CollisionManager` once no consumer remains, updating explicit CMake source lists.
- [ ] Smoke-test projectile/solid interactions, possession, loot, dialogue, swimming, and static terrain reload in the real application. Preserve story-first event ordering.

**M4 acceptance:** Movement/detection/response tests are part of the SDL-free core. No physics source depends on scene headers. Scene adapters contain only orchestration and the game operations deliberately retained here.

---

## M5. Share validated definitions between editor and runtime

### Task 5.1 — Introduce JSON-free item and definition values

**Files:** Create `src/game/items/Item.hpp`, `src/game/items/WeaponConfig.hpp`, `src/world/EntityDefinition.hpp`, `src/serialization/ComponentJson.hpp/.cpp`; modify `src/physics/InventoryManager.hpp`, `src/ecs/Components.hpp`, `src/world/EntityCatalog.hpp/.cpp`, existing component/weapon parser consumers and tests.

**Design contract:** Authored immutable configuration is distinct from mutable runtime state. `Item` remains the runtime inventory value but holds typed optional configuration, not JSON. `WeaponConfig` contains the authored fields currently parsed by `CWeapon`; `CWeapon` contains runtime cooldown plus its initialized configuration/state. Place shared `WeaponType` and collision-mask definitions in small independent headers so items do not include all components or create a cycle.

- [ ] Inventory the actual keys accepted by every existing component constructor, `SpawnFromJSON`, player spawn, `Item`, `loadInventoryFromJson`, and `EntityCatalog`. Record a schema table in `docs/ContentDefinitions.md` with required/optional/default values and units. Preserve player-specific and item-template behavior explicitly.
- [ ] Introduce `Item::weaponConfig` as `std::optional<WeaponConfig>` and `Item::shadowConfig` as `std::optional<ShadowConfig>`. `ShadowConfig` has `float scale = 1.0f; Vec2 offset{0,0};` and lives in `src/game/components/ShadowConfig.hpp`, included by both item and entity definition headers to avoid an item/definition cycle.
- [ ] Remove redundant `hasWeaponConfig`/`hasShadowConfig` flags after migrating every consumer. Preserve `Item::id`, stack/index semantics, active-inventory copy behavior, and existing lookup by item display name until a documented compatibility alias is installed in M5.2.
- [ ] Introduce `VisualDefinition` with `sprite`, `layer`, `repeat`, optional shadow configuration. Introduce `InventoryDefinition` with slot count, active slot, and owned item references containing definition key/quantity/slot. Do not embed live `EntityID`s in any definition.
- [ ] Define `EntityDefinition` with string `id`, display name, source path, optional visual and inventory definitions, and an owned vector of `ComponentDefinition` values. `ComponentDefinition` is a `std::variant` of supported nonvisual spawn components: `CName`, `CAllegiance`, `CTransform`, `CPhysicsBody`, `CItem`, `CCollider`, `CState`, `CHealth`, `CPossessable`, `CInput`, `CCurrency`, `CAIAgent`, and `CEvent`. Each alternative occurs at most once per definition. Authored `CAnimation` and `CShadow` normalize into `VisualDefinition` rather than duplicate variant entries; the factory materializes corresponding runtime components. Use component copies only as initialization templates; reset per-instance mutable fields in the factory. `CInventory` is represented by `InventoryDefinition`, not shared runtime storage.
- [ ] Move JSON constructors into named conversion functions in `ComponentJson.cpp`, e.g. `CPhysicsBody parsePhysicsBody(const nlohmann::json&)` and `WeaponConfig parseWeaponConfig(const nlohmann::json&)`. After migration, runtime value headers do not include `external/json.hpp`. Parser declarations may use the vendored library's forward declarations if supplied; otherwise keep JSON dependency confined to serialization headers.
- [ ] Update tests to distinguish parser tests from runtime-value tests. Preserve cases for physics validation, collider shapes, weapon masks/animation timing, item overrides, and AI defaults. Build after each moved type group to catch circular includes early.

**Acceptance:** No `nlohmann::json` member remains in runtime components/items. Existing data semantics have a written schema table.

### Task 5.2 — Build a transactional definition repository and validator

**Files:** Create `src/world/DefinitionRepository.hpp/.cpp`, `src/serialization/DefinitionParser.hpp/.cpp`, `tests/world/test_definition_repository.cpp`; modify `src/assets/Assets.hpp/.cpp` as needed to expose renderer-independent metadata, `CMakeLists.txt`; register `definition_repository_tests` in core.

**Interfaces:**

```cpp
struct ContentPaths {
    std::filesystem::path mobs;
    std::filesystem::path entities;
    std::filesystem::path items;
    std::filesystem::path assets;
};
struct DefinitionError {
    std::filesystem::path file;
    std::string definition;
    std::string field;
    std::string message;
};
using DefinitionErrors = std::vector<DefinitionError>;
class DefinitionRepository {
public:
    DefinitionErrors load(const ContentPaths& paths);
    const EntityDefinition* findEntity(std::string_view id) const;
    const Item* findItem(std::string_view id) const;
    const std::vector<EntityDefinition>& entities() const noexcept;
};
```

`load` returns an empty error vector only when successful. Parse into temporary storage, validate cross-references, and swap into the live repository only after all checks pass. On failure, the previously loaded repository is unchanged. Lookups do not insert or perform I/O. Returned pointers remain valid until the next **successful** reload; runtime reload is allowed only at a scene boundary, never while a system holds a definition pointer.

- [ ] Parse sorted JSON file lists and collect contextual errors. Wrap parser exceptions with file, definition ID, and JSON field path; do not return an unqualified `json.exception` as the only diagnostic.
- [ ] Validate duplicate entity keys across mobs/entities, duplicate item keys/numeric IDs, root-name mismatch, missing components object, unsupported component names, malformed events, invalid layer names, nonfinite numeric values, nonpositive collider dimensions, negative shadow scale, invalid physics values, and inventory references/slot bounds.
- [ ] Validate sprite/font/weapon animation references against metadata parsed from `assets.json` and its atlas manifests without loading GPU resources. Keep file resolution relative to the documented content root; do not silently depend on an arbitrary current directory.
- [ ] Preserve defaults that are intentionally valid, including `sightRange <= 0` meaning no vision and absence of inventory meaning no attacks. Do not manufacture weapons or collision components to satisfy validation.
- [ ] Use filesystem stem as canonical item definition key while retaining display-name lookup as an explicitly validated compatibility alias for current content. Reject ambiguous aliases; do not let unordered-map iteration select an arbitrary item. Distinguish entity and item namespaces; an item key may resolve through the existing item entity template.
- [ ] Add temporary-directory fixtures serialized with JSON objects. Tests cover valid load, one malformed definition, multiple reported errors, failed reload retaining old data, duplicates, unknown reference, shipped-data load, item aliases, and Linux/Windows path handling.

```cpp
DefinitionRepository repository;
require(repository.load(validPaths).empty(), "valid content rejected");
const auto oldCount = repository.entities().size();
const auto errors = repository.load(invalidPaths);
require(!errors.empty(), "invalid content accepted");
require(repository.entities().size() == oldCount, "failed reload replaced repository");
require(!errors.front().file.empty() && !errors.front().field.empty(),
        "diagnostic lacks source context");
```

`validPaths`/`invalidPaths` refer to fixture folders populated by the test helper; never edit shipped content to induce errors.

- [ ] Add a headless `validate_content` executable under `src/tools/ValidateContent.cpp` taking a content-root argument and returning nonzero on validation errors. Link core only. Register a separate `content_validation_tests` CTest entry that invokes this tool against shipped data; this is a tool invocation, not duplicated per-case registration.

**Acceptance:** Shipped content validates without SDL. Failed reloads preserve the prior valid catalog. Diagnostics identify exact files and fields.

### Task 5.3 — Instantiate definitions with rollback and explicit coordinates

**Files:** Create `src/world/EntityFactory.hpp/.cpp`, `tests/world/test_entity_factory.cpp`; modify `src/scenes/Scene_Play.hpp/.cpp`, `src/scenes/Scene.cpp`, `CMakeLists.txt`; register `entity_factory_tests` in core.

**Interfaces:**

```cpp
enum class SpawnMode { Runtime, EditorPreview };
struct SpawnRequest {
    std::string definition;
    Vec2 worldPosition;
    SpawnMode mode = SpawnMode::Runtime;
};
struct SpawnedEntity {
    EntityID root = InvalidEntity;
    std::vector<EntityID> created;
};
using SpawnResult = std::variant<SpawnedEntity, DefinitionError>;
class EntityFactory {
public:
    explicit EntityFactory(const DefinitionRepository& definitions);
    SpawnResult spawn(ECS& ecs, const SpawnRequest& request) const;
};
```

The factory executes only at a structural barrier and performs no filesystem I/O, rendering, sound playback, or story delivery. `worldPosition` always means world pixels. Callers convert authored grid placements before invoking it, preserving the current grid-to-mid-pixel rule using definition sprite metadata. No ambiguous `Vec2` argument silently switches between tiles and pixels.

- [ ] Test unknown definition returns error with no entity-count change; valid spawn creates independent components; repeated spawns do not share mutable inventory/AI/hit sets; world position and previous position agree initially; runtime mode derives velocity from physics body and records AI spawn position.
- [ ] Use a scoped spawn transaction tracking every allocated handle, including shadow/child entities. Until commit, destructor removes all new entities and hierarchy links on any construction failure. Ensure component assignment failure cannot leave external renderer registration, story events, or inventory changes; defer these side effects until success.
- [ ] Initialize visual components from renderer-independent sprite definitions/handles already parsed into repository metadata. Split `Scene::addVisual` into pure component initialization and scene render-layer registration. The factory may create visual components and shadows headlessly; the scene registers all successful `created` entities afterward.
- [ ] Initialize mutable runtime values explicitly: health/current health according to existing schema, inventory copies and active slot, weapon initial delay as currently intentional, AI state/timers/spawn position, transform history, empty hit sets, no stale parent handles.
- [ ] Add a deterministic test-only injection seam around component construction to throw after root creation and before child completion; verify rollback of both root/child and no external effects. Keep the seam inside factory implementation/test support, not as a general public callback for gameplay.
- [ ] Remove `SpawnFromJSON`'s file access and component-name switch. Its temporary wrapper resolves a `SpawnRequest`, calls the factory, logs contextual errors at the scene boundary, and returns `InvalidEntity` only for legacy callers. Migrate callers to inspect `SpawnResult`, then remove the wrapper.
- [ ] Load definitions once when entering play/editor, sharing the successfully loaded repository with the factory for its lifetime. Do not reload the entire repository for each new scene entity.
- [ ] Add a cache test: load fixture definitions, delete fixture source files, spawn several entities, and verify success. This tests the absence of per-spawn I/O without a benchmark.

**Acceptance:** Spawning is atomic with respect to ordinary content/construction failures; all per-spawn data is owned and independent; renderer/story side effects happen only after success.

### Task 5.4 — Make the editor and runtime consume the same definitions

**Files:** Modify `src/world/EntityCatalog.hpp/.cpp`, `src/scenes/Scene_Editor.hpp/.cpp`, `src/scenes/Scene_Play.cpp`, `src/physics/InventoryManager.hpp`, `tests/world/test_entity_catalog.cpp`, `tests/world/test_entity_factory.cpp`, `docs/ContentDefinitions.md`.

**Contract:** `EntityCatalog` is a read-only editor projection of `DefinitionRepository`, not another parser. Preview instantiation uses `SpawnMode::EditorPreview` and never enables simulation, attacks, quest progression, or audio.

- [ ] Replace catalog directory/JSON parsing with iteration over validated definitions. Preserve category sorting, player/template exclusions, display names, shadow settings, and existing item icon selection.
- [ ] Define preview whitelist explicitly: transform, sprite/animation metadata, shadow hierarchy, display name, and editor placement metadata. No AI, health/status ticking, physics body, weapon, event dispatch, or runtime inventory effects. Selection bounds come from shared visual/collider metadata without activating runtime collision responses.
- [ ] Change `Scene_Editor::spawnPreview` and shadow preview construction to use the factory result and scene registration. Retain unresolved placement entries when content is missing so opening/saving a layout does not silently delete user placements.
- [ ] Make inventory lookup a thin read-only view of repository items; remove its independent directory loading and JSON conversion. Move the remaining class to `src/game/items/InventoryManager.hpp` in M6 after behavior migration is complete.
- [ ] Add parity tests asserting runtime and preview share sprite, layer, transform placement, shadow configuration, and definition key while preview lacks active simulation components. Test unresolved placements survive layout round-trip.
- [ ] Run content validation and all world/ECS suites, then smoke-test opening each editor category, placing/saving/reloading a layout, entering play, inventory use, possession weapon transfer, and spawn shadows.

**M5 acceptance:** Runtime/editor/item loading share a single parser and validated repository. Existing JSON and layout formats still load. No spawn path opens entity/item JSON files.

---

## M6. Enforce the improved structure and conventions

### Task 6.1 — Add Windows, Linux core, and sanitizer CI

**Files:** Create `.github/workflows/ci.yml`; modify `README.md`, `CMakeLists.txt`, and `CMakePresets.json` only where needed for reproducible checks.

**CI jobs:**

| Job | Environment | Commands / policy |
|---|---|---|
| Windows game + tests | `windows-latest`, MSYS2 UCRT64 | Install GCC/CMake/Ninja/SDL packages; configure/build/test Windows debug preset |
| Linux core | `ubuntu-24.04` | Install GCC/CMake/Ninja; run linux-core-debug presets |
| Linux sanitizers | `ubuntu-24.04` | Run linux-core-sanitize presets; fail on sanitizer diagnostics |
| Style/dependencies | `ubuntu-24.04`, Python 3, clang-format 18 | Check dependency rules and changed first-party C++ formatting |

- [ ] Add workflow triggers for pull requests and pushes, with read-only contents permission and cancellation of superseded runs. Use checkout v4 (or the current compatible maintained major at implementation time) and document the chosen action versions.
- [ ] Use `msys2/setup-msys2` with `msystem: UCRT64`, `update: true`, and these install packages:

```yaml
install: >-
  mingw-w64-ucrt-x86_64-gcc
  mingw-w64-ucrt-x86_64-cmake
  mingw-w64-ucrt-x86_64-ninja
  mingw-w64-ucrt-x86_64-sdl3
  mingw-w64-ucrt-x86_64-sdl3-image
  mingw-w64-ucrt-x86_64-sdl3-mixer
  mingw-w64-ucrt-x86_64-sdl3-ttf
```

- [ ] Do not assume the action installs at `C:/msys64`. Discover `MSYS2_LOCATION` supplied by the action and override `CMAKE_CXX_COMPILER` and `CMAKE_MAKE_PROGRAM` at configure time to that location. Prepend its UCRT64 bin to `PATH` and invoke that installation's CMake/CTest explicitly. Preserve local presets' existing default paths for developers.
- [ ] Build tests explicitly through the default build target before CTest. Do not run CTest on a stale cached build. Initial CI needs no build cache; introduce caching only after a reproducible uncached job succeeds.
- [ ] Linux jobs do not install SDL: absence of SDL validates the headless boundary. Sanitizer environment uses `ASAN_OPTIONS=detect_leaks=1` and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1` unless toolchain evidence requires a documented adjustment.
- [ ] Include `validate_content` in the core build/test path. Ensure test fixtures write only temporary directories and never persist changes to shipped content/source mirrors.
- [ ] Resolve CMake's minimum-version messaging: document CMake 3.25 as the preset workflow minimum; if retaining a 3.15 direct-configuration minimum, state that distinction. Correct the Windows failure message that recommends MinGW Makefiles despite Ninja presets.
- [ ] Configure target-local warnings and standards through the existing helper. Keep strict conversion warnings opt-in initially; fix concrete warnings rather than inserting casts solely to silence them. Do not add global `-Werror` until the supported compiler matrix is clean.
- [ ] Verify workflows locally where possible and inspect actual hosted runs when GitHub access is available. Record remote execution as pending if credentials/remote permissions prevent running it.

**Acceptance:** CI reflects documented local commands and does not rely on pre-existing binaries. Every behavior suite from M1–M5 runs in its supported jobs.

### Task 6.2 — Reduce header coupling and finish targeted moves

**Files:** Modify `src/ecs/ECS.hpp`, `src/ecs/ComponentPool.hpp`, `src/ecs/Components.hpp`, `src/core/Game.hpp`, `src/scenes/Scene.hpp`, `CMakeLists.txt`; create `tests/headers/` compile-check sources; move `src/physics/InventoryManager.hpp` to `src/game/items/InventoryManager.hpp`, `src/physics/Level_Loader.hpp/.cpp` to `src/world/LevelLoader.hpp/.cpp`, and `src/physics/Renderer.hpp` to `src/render/RendererManager.hpp`.

**Dependency rules:**

```text
ecs/{ECS,ComponentPool,EntityID,CommandBuffer}.hpp:
    standard library + generic ECS headers only
physics/{CollisionWorld,Contact,Quadtree}:
    no scenes, Game, SDL, or renderer dependencies
game/systems:
    ECS + game values + physics; no scenes/platform headers
serialization:
    JSON + definition/value types; no scenes/render backend
world/{DefinitionRepository,EntityFactory}:
    no SDL, scene, or render backend dependencies
```

- [ ] Remove transitive includes, including `ECS.hpp` from `Game.hpp` if no declared member needs it. Forward-declare scene/platform types when only pointers/references are declared; keep out-of-line destructors where incomplete `unique_ptr` targets require them.
- [ ] Split component value declarations into focused headers: `TransformComponents.hpp`, `PhysicsComponents.hpp`, `VisualComponents.hpp`, `InventoryComponents.hpp`, `AIComponents.hpp`, `HierarchyComponents.hpp`, and `StatusComponents.hpp` under `src/game/components/`. Keep `src/ecs/Components.hpp` as a temporary umbrella for application code, then migrate hot/core headers to direct includes. Do not move unrelated components simply to reach an arbitrary file-size target.
- [ ] Keep JSON converters exclusively in serialization files and stop including the entire component umbrella from generic ECS. Move component-aware debug names/formatting into `EntityInspector`.
- [ ] Perform the three targeted file moves atomically with include/CMake/documentation updates. Use class-oriented names `LevelLoader` and `RendererManager`; preserve runtime behavior.
- [ ] Generate one compile-check translation unit per important public first-party header, each containing only its own include. Include ECS, clock, contact, definition, factory, item, and component headers. Compile core headers with SDL unavailable. Do not rely on one combined translation unit that hides missing includes.
- [ ] Add `scripts/check-dependencies.py` to enforce the listed forbidden direct include edges with clear path/line diagnostics. Keep it a small allowlisted path-rule script, not a C++ parser. Test its rules using in-memory sample include lines; generated/vendor files are excluded.
- [ ] Update README architecture and move references in `AGENTS.md` only if the instructions have become inaccurate. Explain that generic ECS has no content/hierarchy policy and collision discovery has no scene callbacks.

**Acceptance:** Headers compile self-contained; include cycles and forbidden scene/SDL dependencies fail automated checks; SDL-free suites still pass after moves.

### Task 6.3 — Adopt enforceable, incremental C++ conventions

**Files:** Create `.clang-format`, `.clang-tidy`, `docs/CppStandards.md`, `scripts/check-format.py`; modify `.github/workflows/ci.yml`, `README.md`; update only first-party files intentionally migrated by these milestones.

**Conventions:**

- Four-space indentation; select one brace layout and encode it in clang-format (use Allman to match the predominant class/function style).
- `PascalCase` types, `lowerCamelCase` functions/locals, `m_` member prefix for existing classes. Do not rename serialized keys or user-facing action names to match C++ identifiers.
- `override` on overrides; `explicit` on intentional non-converting single-argument constructors; `const` query methods; `[[nodiscard]]` on failure-bearing result APIs; initialize scalar members in-class.
- Own resources with RAII; raw pointers/references are non-owning. Use `unique_ptr` for exclusive ownership and `shared_ptr` only when shared lifetime is required and explained.
- No implicit integer/entity conversions, mutable global lookup maps, or `reinterpret_cast` for ordinary downcasts.
- Headers include what they use. No project-wide `using namespace` in headers. New global aliases must belong to a focused header. Broad namespace migration is deferred.

- [ ] Add `.clang-format` with an explicit base and pinned formatter version 18 in documentation/CI:

```yaml
BasedOnStyle: LLVM
IndentWidth: 4
BreakBeforeBraces: Allman
ColumnLimit: 110
SortIncludes: CaseSensitive
```

- [ ] Add focused clang-tidy checks: `bugprone-*`, `performance-*`, `modernize-use-override`, `modernize-use-nullptr`, `modernize-use-equals-default`, `cppcoreguidelines-pro-type-member-init`. Start advisory for legacy files and gate newly extracted files once clean. Exclude `src/external/` and generated output; never format vendored glad/JSON.
- [ ] Make `check-format.py` accept `--base <git-ref>` and `--all-owned`. With `--base`, obtain added/modified first-party `.hpp/.cpp` files using `git diff --name-only --diff-filter=ACM <base>...HEAD`, inspect complete file formatting with clang-format 18, and print unified diffs without editing files. Ignore deleted files, vendor files, and generated directories. `--all-owned` checks all explicitly adopted files recorded in the script's small path list.
- [ ] CI fetches enough history for the PR merge base. On a push without a usable previous ref, use `--all-owned`. Do not silently skip formatting because a base SHA is unavailable.
- [ ] Format new/extracted modules and any intentionally adopted modified file as a dedicated mechanical step after behavioral tests pass. Avoid a repository-wide reformat that obscures logic changes.
- [ ] Use compile_commands.json for clang-tidy. Document concrete invocation for one extracted source, for example:

```sh
clang-tidy src/game/systems/MovementSystem.cpp -p build/Core/Debug
python3 scripts/check-dependencies.py
python3 scripts/check-format.py --all-owned
```

- [ ] Update README test inventory, prerequisites, content-validator invocation, formatter version, and headless system boundaries. Remove stale claims such as world-view rendering being absent if the current renderer interface already implements it.
- [ ] Run Windows build/tests, Linux core/sanitizer tests, header compile checks, content validator, dependency script, and adopted-file formatting checks. Inspect the final diff for accidental vendor/data/generated changes.

**M6 acceptance:** Standards are written and executable, CI is green on supported platforms, and legacy style debt is explicit rather than masked by broad suppressions.

---

## 5. Final acceptance checklist for the future implementer

- [ ] M1: All original suites pass, including Windows world-layout persistence; initialization failure returns nonzero.
- [ ] M2: Full handles survive every API/cache/hierarchy boundary; stale handles cannot reach recycled entities; structural work owns its arguments and runs at barriers.
- [ ] M3: Simulation advances at 60 Hz independent of render cadence; input buffering and scene transitions have tests; pause/editor behavior is explicit.
- [ ] M4: Movement, contact detection, and contact response have SDL-free tests; physics contains no scene callbacks.
- [ ] M5: Definitions parse once, validate transactionally, and serve runtime/editor equally; failed spawns leave no partial entities or side effects.
- [ ] M6: CI, formatter, focused static analysis, header checks, and dependency checks are documented and exercised.
- [ ] Required possession/input/attack/system-order invariants from section 2 remain covered or explicitly smoke-tested where still scene-bound.
- [ ] No gameplay content/balance/schema change is hidden inside a structural refactor.
- [ ] No unfinished compatibility wrappers, duplicate parsers, or temporary source copies remain.
- [ ] Execution notes distinguish tests actually run from unavailable platform/manual checks.

## 6. Planning self-review and handoff

This file is a plan, not a claim that any milestone has been implemented. At creation, all implementation checkboxes are intentionally unchecked.

Coverage: each of the six requested roadmap items maps to one milestone. Cross-cutting entity, command, clock, movement, contact, definition, factory, and validation contracts are declared before their downstream use. Remaining deliberate scope limits are the full scene-stack/event-bus redesign and full simulation ownership extraction.

Start future work at **Task 1.1**, after checking current instructions/status and reproducing the baseline. If the project has already evolved, reconcile this file against current symbols and tests before editing; preserve the acceptance contracts rather than blindly applying historical line references.

### Execution notes

No implementation has been performed as part of writing this plan.
