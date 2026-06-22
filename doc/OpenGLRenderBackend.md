# OpenGL Render Backend Current State

This document describes the current OpenGL renderer. Older notes described it
as a triangle-only smoke test; that is no longer accurate.

## Summary

The project now has two implementations behind the same `RenderBackend`
interface:

- `SDLRenderBackend`: uses `SDL_Renderer`, `SDL_Texture`, and SDL_ttf surfaces.
- `OpenGLRenderBackend`: uses an SDL-created OpenGL context, GLAD, OpenGL
  textures, a sprite batch, and glyph atlases.

`Game` currently defaults to:

```cpp
RenderDriver m_renderDriver = RenderDriver::OpenGL;
```

The OpenGL backend can currently:

- create an SDL OpenGL window/context
- load OpenGL functions through GLAD
- load textures from image files
- load fonts with SDL_ttf
- build printable ASCII glyph atlases
- draw batched textured sprites
- draw filled rectangles through a 1x1 white texture
- draw rectangle outlines using four filled rectangles
- draw text using glyph atlas quads
- clear and present frames
- resize the viewport and screen projection

It still does not apply the gameplay camera in OpenGL projection space. World
entities are transformed to screen-space by scene code before they are submitted
to the renderer.

## Build Setup

GLAD lives in:

```text
src/external/glad/include/glad/glad.h
src/external/glad/src/glad.c
```

`CMakeLists.txt` builds GLAD as a static library and links it into
`heavenhell`.

The project also links:

```text
SDL3
SDL3_image
SDL3_mixer
SDL3_ttf
```

## SDLPlatform's Role

`SDLPlatform` owns SDL initialization, the window, input polling, audio, and
CPU-side image pixel loading.

When the selected render driver is OpenGL, `SDLPlatform` creates the SDL window
with `SDL_WINDOW_OPENGL` and requests an OpenGL 3.3 core profile:

```cpp
SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
```

The OpenGL context itself is created later by `OpenGLRenderBackend`.

## Game Factory

`Game.cpp` chooses the backend with a small factory:

```cpp
std::unique_ptr<RenderBackend> createRenderBackend(RenderDriver driver, SDLPlatform& platform)
{
    switch (driver) {
    case RenderDriver::SDLRenderer:
        return std::make_unique<SDLRenderBackend>(platform.window());
    case RenderDriver::OpenGL:
        return std::make_unique<OpenGLRenderBackend>(platform.window());
    }
    throw std::runtime_error("Unknown render driver.");
}
```

This keeps scenes and gameplay code talking to `RenderBackend&`, not to SDL or
OpenGL directly.

## What OpenGLRenderBackend Owns

`OpenGLRenderBackend` owns:

- the SDL window pointer it renders into
- the SDL OpenGL context
- current window width and height
- an `OpenGLSpriteBatch`
- OpenGL texture IDs and texture sizes
- SDL_ttf font handles
- OpenGL glyph atlases for text rendering

Texture and font loading are part of `RenderBackend`, so `Assets` can remain a
catalog/metadata loader instead of owning renderer-specific objects.

## Constructor Flow

The constructor:

1. Creates the OpenGL context with `SDL_GL_CreateContext`.
2. Makes the context current with `SDL_GL_MakeCurrent`.
3. Loads OpenGL functions through GLAD using `SDL_GL_GetProcAddress`.
4. Enables vsync with `SDL_GL_SetSwapInterval(1)`.
5. Reads the current window size and sets `glViewport`.
6. Initializes the sprite batch screen projection.
7. Initializes SDL_ttf.
8. Creates sprite batch GPU resources.

The debug triangle path has been removed. Rendering now flows through
`OpenGLSpriteBatch`.

## Frame Flow

`Game::run()` calls:

```cpp
m_renderBackend->beginFrame({0, 0, 0, 255});
update();
sUserInput();
FrametimeHandler();
m_renderBackend->endFrame();
```

In OpenGL mode, `beginFrame`:

- updates the viewport
- clears the color buffer
- updates the screen projection
- starts a new sprite batch

`endFrame` flushes the batch and swaps the SDL OpenGL window buffers.

## Sprite Batch

`OpenGLSpriteBatch` draws quads using instanced rendering.

It owns:

- a shader program
- a static quad vertex buffer
- an index buffer
- an instance buffer
- a VAO
- a 1x1 white texture for solid-color rectangles
- the current screen projection matrix

Each sprite instance stores:

- destination rectangle
- source UVs
- angle
- color

The batch flushes when the texture changes or the maximum batch size is reached.

Current projection maps screen pixels to OpenGL clip space:

```text
screen pixel coordinates -> orthographic clip-space coordinates
```

There is not yet a separate world projection.

## Texture Loading

`OpenGLRenderBackend::loadTexture`:

1. Loads an image with `IMG_Load`.
2. Converts it to `SDL_PIXELFORMAT_RGBA32`.
3. Locks the surface.
4. Creates an OpenGL texture.
5. Sets nearest filtering and clamp-to-edge wrapping.
6. Uploads pixels with `glTexImage2D`.
7. Stores the texture ID and size by asset name.

Nearest filtering is correct for the current pixel-art style.

## Font And Text Rendering

`OpenGLRenderBackend::loadFont` opens a font with SDL_ttf and builds an
`OpenGLGlyphAtlas`.

`OpenGLGlyphAtlas` currently packs printable ASCII characters from 32 to 126
into a 512x512 atlas, falling back to 1024x1024 if needed. Unsupported
characters map to `?`.

`drawText` lays out glyphs using:

- glyph advance
- glyph metrics
- kerning from SDL_ttf
- the requested destination rectangle

Each glyph becomes a textured quad in the sprite batch.

## Rectangles

`fillRect` draws a tinted quad using the sprite batch white texture.

`drawRect` draws four thin filled rectangles. This is simple and adequate for
debug/collision drawing right now.

## Current Limitations

- World camera projection is still done in scene code, not in the renderer.
- There is no `RenderView`, `drawWorldSprite`, `drawWorldRect`, or
  `drawWorldText` API yet.
- The sprite batch has one screen projection, not separate screen/world
  projections.
- Render driver selection is hard-coded in `Game.hpp`.
- Shaders are embedded C++ strings rather than external assets.
- Text only supports printable ASCII in the OpenGL glyph atlas.
- There are no automated renderer smoke tests.

## Recommended Next Step

The next backend milestone should be moving world camera projection into the
renderer:

```text
screen = (world - camera) * scale + origin
```

SDL can still apply that transform on the CPU inside `SDLRenderBackend`, while
OpenGL should apply it through a world projection matrix in the vertex shader.

See [WorldSpaceRendering.md](WorldSpaceRendering.md) and
[BackendSetupRoadmap.md](BackendSetupRoadmap.md).
