# World-Space Rendering

World-space rendering is implemented. The renderer now owns conversion from
world coordinates to display coordinates, while game code continues to own
camera state.

## Render Spaces

- Screen space is for menus, HUD elements, inventory, pause overlays, and FPS
  text. Existing `drawSprite`, `drawRect`, `fillRect`, and `drawText` methods
  remain screen-space APIs.
- World space is for entities, world dialog, collision/interaction debug
  boxes, quadtree overlays, and enemy damage hearts. These use explicit
  `drawWorld*` methods.

`RenderView` describes the active world transform:

```text
screen = (world - camera) * scale + origin
```

It contains camera position, scale, and origin, plus pure helpers that convert
world points and rectangles to screen space. The current scene preserves the
previous zoom behavior exactly:

```text
scale  = gameScale - cameraZoom
origin = virtualScreenCenter * cameraZoom
```

## RenderBackend API

`RenderBackend` exposes the following world-space operations:

```cpp
setWorldView(const RenderView& view)
drawWorldSprite(const WorldSpriteDrawCommand& command)
drawWorldRect(const RectF& rect, Color color)
fillWorldRect(const RectF& rect, Color color)
drawWorldText(const WorldTextDrawCommand& command)
```

World sprite and text commands use a top-left `RectF dst` in world units.
There is intentionally no general sprite tint API yet; existing sprites are
drawn with their original colors.

## Camera And Scene Responsibilities

`Camera` is unchanged. It still owns follow movement, pan, shake, and zoom.
It does not depend on a renderer or OpenGL types.

`Scene` builds one `RenderView` from the current camera and game scale, gives
it to the render backend, and decides whether a draw belongs to the world or
the UI. `Scene::sRenderBasic()` now submits entity sprites, dialog text, and
debug boxes in world coordinates. `Scene_Play` submits enemy damage hearts and
quadtree outlines in world coordinates while keeping player hearts and the
inventory in screen space.

## SDL Backend

`SDLRenderBackend` stores the active `RenderView`. Each world draw command is
converted to the existing screen-space equivalent on the CPU and then uses the
normal SDL renderer path. This keeps SDL behavior aligned with the old scene
math and makes it a useful reference implementation.

## OpenGL Backend

`OpenGLSpriteBatch` keeps independent screen and world projection matrices.
The world matrix includes the `RenderView` transform and maps world coordinates
directly to OpenGL clip space. The batch tracks whether queued quads are screen
or world space and flushes when the texture, render space, or active world view
would change.

World rectangle outlines use `1 / view.scale` world units in OpenGL, yielding
the same one-screen-pixel thickness as SDL. `fillWorldRect` ignores invalid
rectangles; the screen-space `fillRect` full-window shorthand is unchanged.

## Verification

- `render_view_tests` covers world point conversion, rectangle conversion, and
  the current camera-zoom origin behavior.
- Build with the Windows CMake debug preset and run CTest for automated
  coverage.
- Manual OpenGL checks should cover camera follow, shake, pan, zoom, dialogs,
  collision/interactions, quadtree outlines, and fixed HUD/UI elements.

## Not Included

- Runtime SDL/OpenGL driver selection remains a separate backend roadmap item.
- Camera-model cleanup, world culling, static command caching, and future 3D
  view/projection work remain future improvements.
