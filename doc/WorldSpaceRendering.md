# Moving Camera Transform Into The Renderer

This guide explains how to move camera-to-screen transform logic out of
`Scene::sRenderBasic()` and into the rendering backends.

The goal is not for the renderer to own the gameplay `Camera` object. The
camera should still update position, zoom, shake, and pan in scene/game logic.
The renderer should receive a small view description and apply it consistently.

## 1. Target Shape

Keep two kinds of rendering:

- Screen-space rendering: HUD, FPS text, menus, inventory, pause overlays.
- World-space rendering: level sprites, enemies, player, world dialog, collision
  debug boxes, interaction boxes.

Screen-space rendering uses the existing API:

```cpp
drawSprite(...)
drawText(...)
drawRect(...)
fillRect(...)
```

World-space rendering gets new API:

```cpp
setWorldView(...)
drawWorldSprite(...)
drawWorldText(...)
drawWorldRect(...)
fillWorldRect(...)
```

This keeps UI independent from the camera while allowing OpenGL to apply world
camera transforms in the vertex shader.

## 2. Add Render Types

Edit `src/render/RenderTypes.hpp`.

Add a `RenderView`:

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

`RenderView` preserves the current scene math:

```cpp
screen = (world - camera) * scale + origin
```

Add a world sprite command:

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
```

Add world text command:

```cpp
struct WorldTextDrawCommand
{
    std::string text;
    std::string fontName;
    RectF dst;
    Color color = {255, 255, 255, 255};
};
```

Use `RectF` directly for world rectangles. In world space, `RectF::x/y` means
world top-left and `RectF::w/h` means world size.

## 3. Extend RenderBackend

Edit `src/render/RenderBackend.hpp`.

Add these pure virtual methods:

```cpp
virtual void setWorldView(const RenderView& view) = 0;
virtual void drawWorldSprite(const WorldSpriteDrawCommand& command) = 0;
virtual void drawWorldRect(const RectF& rect, Color color) = 0;
virtual void fillWorldRect(const RectF& rect, Color color) = 0;
virtual void drawWorldText(const WorldTextDrawCommand& command) = 0;
```

Both SDL and OpenGL must implement these before the project will compile.

## 4. Implement World View In SDL

Edit:

```text
src/render/sdl/SDLRenderBackend.hpp
src/render/sdl/SDLRenderBackend.cpp
```

Add a member:

```cpp
RenderView m_worldView;
```

Add helpers:

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

Implement:

```cpp
void SDLRenderBackend::setWorldView(const RenderView& view)
{
    m_worldView = view;
}
```

Implement `drawWorldSprite` by converting center/size to a screen-space `dst`
and then using the existing SDL sprite path:

```cpp
void SDLRenderBackend::drawWorldSprite(const WorldSpriteDrawCommand& command)
{
    const float screenW = command.width * m_worldView.scale;
    const float screenH = command.height * m_worldView.scale;
    const float screenCenterX = worldToScreenX(command.centerX);
    const float screenCenterY = worldToScreenY(command.centerY);

    drawSprite(SpriteDrawCommand{
        command.texture,
        command.src,
        RectF{
            screenCenterX - screenW * 0.5f,
            screenCenterY - screenH * 0.5f,
            screenW,
            screenH
        },
        command.angle
    });
}
```

Implement the rectangle methods:

```cpp
void SDLRenderBackend::drawWorldRect(const RectF& rect, Color color)
{
    drawRect(worldRectToScreen(rect), color);
}

void SDLRenderBackend::fillWorldRect(const RectF& rect, Color color)
{
    fillRect(worldRectToScreen(rect), color);
}
```

Implement world text by converting the destination rect and delegating to
`drawText`:

```cpp
void SDLRenderBackend::drawWorldText(const WorldTextDrawCommand& command)
{
    drawText(TextDrawCommand{
        command.text,
        command.fontName,
        worldRectToScreen(command.dst),
        command.color
    });
}
```

SDL still does the world-to-screen math on the CPU. That is fine. The benefit is
that the math is now contained in the backend and the scene code becomes
backend-neutral.

## 5. Add Render Space To OpenGLSpriteBatch

Edit:

```text
src/render/opengl/OpenGLSpriteBatch.hpp
src/render/opengl/OpenGLSpriteBatch.cpp
```

Add an enum:

```cpp
enum class OpenGLRenderSpace
{
    Screen,
    World
};
```

Add members:

```cpp
std::array<float, 16> m_screenProjection;
std::array<float, 16> m_worldProjection;
OpenGLRenderSpace m_currentSpace = OpenGLRenderSpace::Screen;
OpenGLRenderSpace m_pendingSpace = OpenGLRenderSpace::Screen;
```

You can rename the existing `m_projection` to `m_screenProjection`.

Add:

```cpp
void setWorldView(const RenderView& view);
```

Change `drawTexturedQuad` to accept a render space:

```cpp
void drawTexturedQuad(
    unsigned int textureId,
    TextureSize textureSize,
    const RectF& src,
    const RectF& dst,
    float angle,
    Color color,
    OpenGLRenderSpace space);
```

Inside `drawTexturedQuad`, flush when either the texture or render space changes:

```cpp
if (m_batchCount > 0 && (m_currentTexture != textureId || m_currentSpace != space)) {
    flush();
}

m_currentTexture = textureId;
m_currentSpace = space;
```

In `flush`, choose the matrix based on `m_currentSpace`:

```cpp
const auto& projection =
    (m_currentSpace == OpenGLRenderSpace::World)
        ? m_worldProjection
        : m_screenProjection;

glUniformMatrix4fv(m_projectionUniform, 1, GL_FALSE, projection.data());
```

Screen-space calls use `OpenGLRenderSpace::Screen`. World-space calls use
`OpenGLRenderSpace::World`.

## 6. Build The OpenGL World Projection

The current screen projection maps screen pixels to clip space:

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

The world projection should include this transform:

```text
screenX = (worldX - cameraX) * scale + originX
screenY = (worldY - cameraY) * scale + originY
```

So the world projection matrix is:

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

This lets the shader receive world coordinates directly. No per-sprite camera
subtraction is needed on the CPU for OpenGL.

## 7. Implement World Methods In OpenGLRenderBackend

Edit:

```text
src/render/opengl/OpenGLRenderBackend.hpp
src/render/opengl/OpenGLRenderBackend.cpp
```

Add:

```cpp
RenderView m_worldView;
```

Implement:

```cpp
void OpenGLRenderBackend::setWorldView(const RenderView& view)
{
    m_worldView = view;
    m_spriteBatch.setWorldView(view);
}
```

Implement `drawWorldSprite`:

```cpp
void OpenGLRenderBackend::drawWorldSprite(const WorldSpriteDrawCommand& command)
{
    const OpenGLTexture& texture = getTexture(command.texture);
    m_spriteBatch.drawTexturedQuad(
        texture.id,
        texture.size,
        command.src,
        RectF{
            command.centerX - command.width * 0.5f,
            command.centerY - command.height * 0.5f,
            command.width,
            command.height
        },
        command.angle,
        command.color,
        OpenGLRenderSpace::World
    );
}
```

Implement `drawWorldRect` and `fillWorldRect` the same way as screen-space
rectangles, but pass `OpenGLRenderSpace::World` to the batch.

Implement `drawWorldText` by reusing the existing glyph atlas layout code, but
send each glyph quad through the batch as world-space. The text command `dst`
should be in world units.

Keep existing `drawSprite`, `drawText`, `drawRect`, and `fillRect` screen-space
and make them pass `OpenGLRenderSpace::Screen`.

## 8. Migrate Scene::sRenderBasic

Edit:

```text
src/scenes/Scene.cpp
```

At the top of `Scene::sRenderBasic`, keep the current camera values but store
them as a `RenderView`:

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

This preserves the current formula exactly:

```cpp
screen = (world - camera) * totalZoom + screenCenterZoomed
```

Replace world sprite screen-space math:

```cpp
Vec2 adjustedPosition = (transform.pos - m_camera.position) * totalZoom + screenCenterZoomed;
Vec2 destSize = sprite.size() * transform.scale * totalZoom;
RectF dst{...};
drawSprite(sprite, dst, transform.angle);
```

with:

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

Replace world text:

```cpp
Vec2 pos = (transform.pos - m_camera.position - dialog.size / 2) * totalZoom + screenCenterZoomed;
drawText({ ..., RectF{pos.x, pos.y, dialog.size.x * totalZoom, dialog.size.y * totalZoom}, ... });
```

with:

```cpp
m_game->render().drawWorldText(WorldTextDrawCommand{
    dialog.text,
    dialog.font_name,
    RectF{
        transform.pos.x - dialog.size.x * 0.5f,
        transform.pos.y - dialog.size.y * 0.5f,
        dialog.size.x,
        dialog.size.y
    },
    {255, 255, 255, 255}
});
```

Replace collision/debug boxes:

```cpp
RectF boxRect{
    transform.pos.x - box.halfSize.x,
    transform.pos.y - box.halfSize.y,
    box.size.x,
    box.size.y
};
m_game->render().drawWorldRect(boxRect, box.color);
```

## 9. Keep UI Screen-Space

Do not convert HUD and menu overlay code unless it is intentionally part of the
world.

Keep these using screen-space methods:

- FPS counter
- hearts
- inventory UI
- pause overlay
- game over overlay
- finish overlay

For screen-space code, continue using:

```cpp
drawSprite(...)
drawText(...)
drawRect(...)
fillRect(...)
```

This prevents UI from moving when the camera moves.

## 10. Build And Test

Before building, close any running `heavenhell.exe`. Windows will refuse to
overwrite the executable if the game is still open.

Build:

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
$env:TMP='C:\repos\Heaven-Hell\build\tmp'
$env:TEMP='C:\repos\Heaven-Hell\build\tmp'
C:\msys64\ucrt64\bin\ninja.exe -C build
```

Test SDL and OpenGL:

- Player and world sprites appear in the same positions as before.
- Camera movement still follows the player.
- Camera shake still affects world sprites.
- Camera pan still affects world sprites.
- Zoom still affects world sprites.
- Collision and interaction debug boxes still line up.
- Dialog text follows world entities.
- HUD hearts, inventory, pause overlay, and FPS counter do not move with camera.
- SDL and OpenGL remain visually close enough.

## 11. Expected Result

After this change:

- `Scene::sRenderBasic()` no longer performs per-sprite camera subtraction.
- SDL still performs world-to-screen math on the CPU, but inside the SDL backend.
- OpenGL applies the world camera transform through the projection matrix.
- Screen-space UI remains unaffected by the world camera.
- The renderer has a cleaner path toward future 3D concepts:
  `projection * view * model`.

## 12. Future Improvements

After this works, consider these follow-ups:

- Add explicit `beginWorldPass()` and `beginUiPass()` if render state grows.
- Add a `RenderCamera` or `ViewMatrix` type when 3D starts.
- Cache static world sprite commands for stationary tiles/entities.
- Add frustum/camera culling before sending world draw commands.
- Keep SDL as a compatibility backend, but let OpenGL become the primary engine
  renderer.
