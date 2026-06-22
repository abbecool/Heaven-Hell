# Moving Camera Transform Into The Renderer

This document describes a planned refactor. It has not been implemented yet.

## Current State

World-space rendering is still converted to screen-space in scene code.

`Scene::sRenderBasic()` currently computes:

```text
screen = (world - camera) * totalZoom + screenCenterZoomed
```

Then it submits ordinary screen-space `SpriteDrawCommand`, `TextDrawCommand`,
and rectangle commands to `RenderBackend`.

`Scene_Play::sRender()` also performs camera math for enemy damage hearts and
quadtree debug rendering.

The OpenGL renderer currently receives screen-space rectangles and applies only
a screen-pixel orthographic projection in `OpenGLSpriteBatch`.

## Goal

Keep two kinds of rendering:

- Screen-space rendering: HUD, FPS text, menus, inventory, pause overlays.
- World-space rendering: level sprites, enemies, player, world dialog,
  collision debug boxes, interaction boxes, and quadtree debug overlays.

The renderer should not own the gameplay `Camera` object. Scene/game logic
should still update camera position, zoom, shake, and pan. The renderer should
receive a small view description and apply it consistently.

## Proposed Render Types

Add to `src/render/RenderTypes.hpp`:

```cpp
struct RenderView
{
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    float scale = 1.0f;
    float originX = 0.0f;
    float originY = 0.0f;
};
```

This preserves the current scene formula:

```text
screen = (world - camera) * scale + origin
```

Add world-space commands:

```cpp
struct WorldSpriteDrawCommand
{
    TextureHandle texture;
    RectF src;
    float centerX = 0.0f;
    float centerY = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float angle = 0.0f;
    Color color = {255, 255, 255, 255};
};

struct WorldTextDrawCommand
{
    std::string text;
    std::string fontName;
    RectF dst;
    Color color = {255, 255, 255, 255};
};
```

Use `RectF` directly for world rectangles. In world-space, `RectF::x/y` means
world top-left and `RectF::w/h` means world size.

## Proposed RenderBackend API

Add to `src/render/RenderBackend.hpp`:

```cpp
virtual void setWorldView(const RenderView& view) = 0;
virtual void drawWorldSprite(const WorldSpriteDrawCommand& command) = 0;
virtual void drawWorldRect(const RectF& rect, Color color) = 0;
virtual void fillWorldRect(const RectF& rect, Color color) = 0;
virtual void drawWorldText(const WorldTextDrawCommand& command) = 0;
```

Both SDL and OpenGL backends must implement these before the project compiles.

## SDL Implementation

`SDLRenderBackend` can keep doing the world-to-screen math on the CPU.

Add:

```cpp
RenderView m_worldView;
```

Helpers:

```cpp
float worldToScreenX(float x) const
{
    return (x - m_worldView.cameraX) * m_worldView.scale + m_worldView.originX;
}

float worldToScreenY(float y) const
{
    return (y - m_worldView.cameraY) * m_worldView.scale + m_worldView.originY;
}

RectF worldRectToScreen(const RectF& rect) const
{
    return RectF{
        worldToScreenX(rect.x),
        worldToScreenY(rect.y),
        rect.w * m_worldView.scale,
        rect.h * m_worldView.scale
    };
}
```

Then world methods can convert to screen-space and delegate to the existing
screen-space methods.

## OpenGL Implementation

`OpenGLSpriteBatch` currently has a single `m_projection`. It should gain a
screen projection and a world projection.

Add a render-space enum:

```cpp
enum class OpenGLRenderSpace
{
    Screen,
    World
};
```

Then track:

```cpp
std::array<float, 16> m_screenProjection;
std::array<float, 16> m_worldProjection;
OpenGLRenderSpace m_currentSpace = OpenGLRenderSpace::Screen;
```

Flush the batch when either the texture or render space changes.

## World Projection Matrix

The existing screen projection maps screen pixels to clip space:

```cpp
std::array<float, 16> makeScreenProjection(float width, float height)
{
    return {
        2.0f / width, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / height, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
}
```

The world projection should include:

```text
screenX = (worldX - cameraX) * scale + originX
screenY = (worldY - cameraY) * scale + originY
```

Matrix:

```cpp
std::array<float, 16> makeWorldProjection(
    float width,
    float height,
    const RenderView& view)
{
    const float safeWidth = width > 0.0f ? width : 1.0f;
    const float safeHeight = height > 0.0f ? height : 1.0f;
    const float scale = view.scale;

    return {
        2.0f * scale / safeWidth, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f * scale / safeHeight, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        ((view.originX - view.cameraX * scale) * 2.0f / safeWidth) - 1.0f,
        1.0f - ((view.originY - view.cameraY * scale) * 2.0f / safeHeight),
        0.0f,
        1.0f
    };
}
```

This lets the shader receive world coordinates directly.

## Scene Migration

At the top of `Scene::sRenderBasic()`, build a `RenderView` from the current
camera values:

```cpp
Vec2 screenCenter = Vec2{(float)width(), (float)height()} / 2;
int windowScale = m_game->getScale();
int totalZoom = windowScale - m_camera.getCameraZoom();
Vec2 screenCenterZoomed = screenCenter * m_camera.getCameraZoom();

m_game->render().setWorldView(RenderView{
    m_camera.position.x,
    m_camera.position.y,
    static_cast<float>(totalZoom),
    screenCenterZoomed.x,
    screenCenterZoomed.y
});
```

Then replace per-sprite camera subtraction with `drawWorldSprite`.

For an entity:

```cpp
Vec2 worldSize = sprite.size() * transform.scale;
m_game->render().drawWorldSprite(WorldSpriteDrawCommand{
    sprite.texture,
    sprite.src,
    transform.pos.x,
    transform.pos.y,
    worldSize.x,
    worldSize.y,
    transform.angle,
    {255, 255, 255, 255}
});
```

World text and debug boxes should use `drawWorldText` and `drawWorldRect`.

## Keep UI Screen-Space

Do not convert HUD and menu overlay code unless it is intentionally part of the
world.

Keep these screen-space:

- FPS counter
- player hearts or future health bar
- inventory UI
- pause overlay
- game over overlay
- finish overlay
- menus

## Test Plan

Before building, close any running `heavenhell.exe`; Windows will refuse to
overwrite a running executable.

Build:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
cmake --build build/Windows/Ninja/Release
```

Verify both SDL and OpenGL after adding configurable render-driver selection:

- World sprites appear in the same positions as before.
- Camera follow, shake, pan, and zoom still work.
- Collision and interaction debug boxes still line up.
- Quadtree debug overlays still line up.
- Dialog text follows world entities.
- HUD hearts, inventory, pause overlay, and FPS counter do not move with the
  camera.
- OpenGL and SDL are visually close enough for the current 2D game.

## Expected Result

After this change:

- `Scene::sRenderBasic()` no longer performs per-sprite camera subtraction.
- SDL performs world-to-screen math inside the SDL backend.
- OpenGL applies the world camera transform through a projection matrix.
- Screen-space UI remains unaffected by the world camera.
- The renderer has a cleaner path toward future 3D concepts:
  `projection * view * model`.

## Future Improvements

- Add explicit `beginWorldPass()` and `beginUiPass()` if render state grows.
- Add frustum/camera culling before sending world draw commands.
- Cache static world sprite commands for stationary tiles/entities.
- Keep SDL as a compatibility backend while OpenGL becomes the main engine
  renderer.
