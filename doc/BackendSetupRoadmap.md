# Backend And Setup Roadmap

This is the backend/setup work worth considering before shifting focus back to
game features. It is based on the current repo state, not the older historical
notes in `context.md`.

## Already Implemented

- SDL platform split:
  `SDLPlatform` owns SDL init/quit, the window, input polling, fullscreen/window
  sizing, audio loading/playback, and CPU-side image pixel loading.
- Renderer abstraction:
  scenes call `RenderBackend` methods instead of SDL or OpenGL directly.
- Renderer-owned resources:
  texture and font resources live in the render backend, not in `Assets`.
- Renderer-neutral assets:
  `Assets` loads the catalog and asks `RenderBackend` to load textures/fonts.
- Sprite data split:
  static sprite metadata lives in `SpriteDefinition`, renderable sprite state in
  `CSprite`, and animation playback state in `CAnimation`.
- Input abstraction:
  SDL key/mouse codes are translated to `InputCode` in `SDLPlatform`; scenes
  register actions using `InputCode`.
- OpenGL 2D renderer:
  OpenGL mode creates a context, loads GLAD, batches textured quads, draws
  sprites/rectangles, and renders text through glyph atlases.
- Release packaging:
  `scripts/package-release.ps1` builds a release and copies runtime DLL
  dependencies into a zip.
- CMake presets and starter tests:
  `CMakePresets.json` defines Windows/Linux debug and release presets, and
  CTest runs lightweight `Vec2`, `RandomArray`, `SpriteDefinition`, and ECS
  component-pool suites plus pure `RenderView` transform coverage.
- World-space rendering:
  `RenderView` and explicit world draw commands keep camera projection inside
  the render backends. SDL performs the transform on the CPU and OpenGL applies
  it through a world projection matrix. See [WorldSpaceRendering.md](WorldSpaceRendering.md).

## Highest Priority

### 1. Make Render Driver Selection Configurable

Current state:

```cpp
RenderDriver m_renderDriver = RenderDriver::OpenGL;
```

That is fine for development, but it makes backend comparison awkward.

Recommended work:

- Add a small runtime config option, command-line arg, or debug setting for
  `SDLRenderer` vs `OpenGL`.
- Keep OpenGL as the default once it is stable.
- Add a simple startup log line showing the selected backend.

Why now:

- You can verify backend changes against the SDL reference path.
- It makes OpenGL regressions easier to isolate.

### 2. Expand Tests

Current state:

- `tests/vec2/test_vec2.cpp` has dependency-free `Vec2` coverage.
- CTest also covers deterministic `RandomArray` behavior, `SpriteDefinition`
  frame calculations, ECS/component-pool lifecycle behavior, and pure
  `RenderView` world-to-screen conversion.

Recommended work:

- Add tests for `ComponentPool`, `SpriteDefinition`, and pure `LevelLoader`
  helpers.
- Keep using assert-based tests until the test surface is large enough to
  justify Catch2 or doctest.
- Keep renderer tests as manual/smoke tests for now unless you add an offscreen
  path later.

Why now:

- Engine/game separation will be easier if math/ECS/resource assumptions have
  basic coverage.

## Important Setup Work

### 4. Keep CMake Presets As The Single Build Entry Point

Current state:

- `CMakePresets.json` defines Windows/Linux debug and release presets.
- VS Code tasks, README, and package script use presets.

Recommended work:

- Keep future build options in `CMakePresets.json`.
- Avoid adding duplicate configure/build argument lists to VS Code tasks or
  scripts.

Why now:

- It removes duplicated build knowledge.
- It makes future engine/game targets easier to configure consistently.

### 5. Add Compile Warnings

Current state:

- `CMakeLists.txt` sets C++20 but does not define warning levels.

Recommended work:

- Add warning options for GCC/Clang.
- Consider warnings-as-errors later, not immediately.
- Start with warnings that are useful but not noisy for this codebase.

Why now:

- Backend refactors are exactly where hidden type conversions and lifetime
  issues tend to surface.

### 6. Tighten Release Packaging

Current state:

- The package script copies all of `assets` and `config_files`.
- That includes source art folders, old assets, unused assets, and a very large
  WAV music file.

Recommended work:

- Create a packaging manifest or curated asset copy list.
- Decide whether source files belong in release zips.
- Prefer OGG over WAV for release music if quality is acceptable.
- Keep the current dependency scanning logic; it is useful.

Why now:

- Release packages will stay smaller and cleaner as assets grow.

## Engine/Game Separation

You already know you want this eventually. It does not have to happen before
new gameplay, but the repo is close enough that planning the boundary now will
save churn.

### 7. Split Targets Before Splitting Repositories

Do this inside one repo first.

Suggested target shape:

```text
heavenhell_engine
  core except Game-specific startup
  render
  assets resource/catalog code
  ecs
  math/physics primitives
  platform
  generic scene contracts

heavenhell_game
  main.cpp
  Game
  concrete scenes
  story/quests
  game-specific components/systems
  config files and assets
```

Why one repo first:

- You can move boundaries without package/versioning friction.
- CMake can enforce the dependency direction before a physical repo split.

### 8. Decide What Is Engine And What Is Game

Likely engine:

- `core/SDLPlatform`
- `core/InputCode`
- `core/PixelImage`
- `render`
- `ecs`
- `assets/SpriteDefinition`
- reusable math like `Vec2`
- generic collision/quadtree pieces after cleanup

Likely game:

- `Scene_Play`, menus, pause, finish, game over
- `Game` as currently written
- story and quest content
- item/mob config loaders that know Heaven-Hell data
- specific components like inventory, possession, story events, and weapon
  tuning

Needs thought:

- `physics/Level_Loader` is currently game/world-loading code, not really
  physics.
- `Components.hpp` mixes reusable ECS component ideas with game-specific
  gameplay data.
- `Assets` currently coordinates render and audio loading; it may need to
  become a generic asset catalog plus game-owned content loading.

### 9. Separate Audio From Platform

Current state:

- `SDLPlatform` owns SDL_mixer initialization and loaded audio maps.
- `Assets` calls `platform.loadAudio()` and `platform.loadMusic()`.

Recommended work:

- Add a small `AudioBackend` or `AudioSystem`.
- Keep SDL_mixer as the implementation.
- Let `Assets` remain catalog-oriented.

Why not through `RenderBackend`:

- Audio is not rendering.
- Keeping it separate makes engine reuse cleaner.

### 10. Move Level Loading Out Of Physics

Current state:

- `Level_Loader` lives in `src/physics` but handles pixel-map world loading,
  tile selection, chunk loading, and entity spawning.

Recommended work:

- Move it toward `src/world` or game-side level code.
- Split pure tile-map logic from `Scene_Play` entity spawning.
- Keep `PixelImage` as the platform-neutral input.

Why now:

- This is one of the clearer current engine/game boundary leaks.

## Rendering Quality And Performance

### 11. Cache SDL Text Rendering

Current state:

- `SDLRenderBackend::drawText` creates and destroys a texture each call.
- OpenGL text already uses glyph atlases.

Recommended work:

- Cache SDL text textures by font/text/color or accept SDL as a slower reference
  backend and prioritize OpenGL.
- At minimum, cache FPS text until its value changes.

### 12. Improve OpenGL Batch Behavior

Current state:

- The batch flushes on texture change.
- Layer order is preserved by scene-side ordering.

Recommended work:

- Consider texture atlases for commonly drawn sprites.
- Keep layer correctness first.
- Add metrics/logging for batch flush count before optimizing deeply.

### 13. Externalize Shaders Later

Current state:

- Shaders are embedded strings in `OpenGLSpriteBatch.cpp`.

Recommended work:

- Leave them embedded while the shader set is tiny.
- Move to shader files once you have more than a couple of shader programs or
  want hot reload.

## Lower Priority Cleanup

- Convert `Scene_Play::loadConfig` from text parsing to JSON.
- Reduce `Scene_Play.cpp` size by extracting systems or helpers.
- Replace the remaining old/historical docs and assets with curated references.
- Add basic logging levels instead of scattered `std::cout`.
- Review `Quadtree` ownership; it still uses `shared_ptr` internally in places.

## Suggested Order

1. Add configurable render-driver selection.
2. Verify SDL/OpenGL visual parity using the world-space rendering path.
3. Clean release packaging.
4. Split CMake targets into engine and game inside this repo.
5. Move audio and level loading to cleaner modules.
6. Only then consider a physical engine/game repo split.
