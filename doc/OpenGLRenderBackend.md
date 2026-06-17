# OpenGLRenderBackend Current State

This document explains how OpenGL rendering is currently set up in the project.
It assumes you already understand the SDL renderer and want to map those ideas
to the new OpenGL backend.

The OpenGL renderer now does real sprite drawing. It still does not render text
or debug rectangles, but `drawSprite` is implemented with batched textured
OpenGL quads.

## Current Capabilities

OpenGL mode can currently:

- create an SDL window with OpenGL support
- create an OpenGL context
- initialize GLAD
- load image assets into OpenGL textures
- remember texture sizes for `SpriteDefinition`
- clear and present frames
- render sprites from `SpriteDrawCommand`
- batch consecutive sprites that use the same texture
- support source rectangles from sprite sheets
- support destination rectangles in the same pixel coordinate style as SDL
- support sprite rotation around the destination rectangle center
- use alpha blending for transparent PNG sprites
- avoid copying render layers every frame by returning layer storage by const
  reference

Still missing:

- `loadFont` does not load any font data yet
- `drawText`
- `drawRect`
- `fillRect`
- camera/projection abstraction
- a proper shader/resource wrapper

## Build Setup

GLAD is built as a small static library from the vendored source:

```cmake
set_source_files_properties(src/external/glad/src/glad.c PROPERTIES LANGUAGE CXX)
add_library(glad STATIC
    src/external/glad/src/glad.c
)

target_include_directories(glad PUBLIC
    src/external/glad/include
)
```

The executable links against `glad` together with SDL:

```cmake
target_link_libraries(heavenhell PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_mixer::SDL3_mixer
    SDL3_ttf::SDL3_ttf
    glad
)
```

The project currently compiles `glad.c` as C++ because the CMake project is
configured with `LANGUAGES CXX`. If the project later enables C as a language,
GLAD can be compiled as normal C instead.

## How The Renderer Is Selected

The game owns a `RenderBackend` pointer. `Game.cpp` creates either the SDL
backend or OpenGL backend:

```cpp
case RenderDriver::SDLRenderer:
    return std::make_unique<SDLRenderBackend>(platform.window());
case RenderDriver::OpenGL:
    return std::make_unique<OpenGLRenderBackend>(platform.window());
```

The rest of the game keeps calling the same interface:

```cpp
m_renderBackend->beginFrame({0, 0, 0, 255});
update();
sUserInput();
FrametimeHandler();
m_renderBackend->endFrame();
```

That means scene code does not need to know whether sprites are drawn by
`SDL_RenderTextureRotated` or by OpenGL.

At the time of writing, `Game.hpp` defaults to OpenGL:

```cpp
RenderDriver m_renderDriver = RenderDriver::OpenGL;
```

## SDLPlatform's OpenGL Role

`SDLPlatform` still owns the window. When `RenderDriver::OpenGL` is selected,
it requests an OpenGL 3.3 core-profile window:

```cpp
SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
windowFlags = static_cast<SDL_WindowFlags>(windowFlags | SDL_WINDOW_OPENGL);
```

This only makes the window capable of OpenGL. The actual OpenGL context is
created later by `OpenGLRenderBackend`.

## What OpenGLRenderBackend Owns

The backend owns:

```cpp
SDL_Window* m_window = nullptr;
SDL_GLContext m_context = nullptr;
int m_width = 1;
int m_height = 1;
unsigned int m_spriteProgram = 0;
unsigned int m_spriteVertexArray = 0;
unsigned int m_spriteVertexBuffer = 0;
size_t m_spriteVertexBufferCapacity = 0;
size_t m_spriteCount = 0;
size_t m_spriteBatchCount = 0;
size_t m_spriteDrawCallCount = 0;
size_t m_spriteTextureBindCount = 0;
std::map<std::string, OpenGLTexture> m_textures;
std::vector<SpriteVertex> m_spriteVertices;
std::vector<SpriteBatch> m_spriteBatches;
```

The important OpenGL objects are:

- `m_context`: the active OpenGL context for the SDL window.
- `m_spriteProgram`: the linked shader program used for sprite drawing.
- `m_spriteVertexArray`: the VAO that describes sprite vertex layout.
- `m_spriteVertexBuffer`: the dynamic VBO used when flushing sprite batches.
- `m_spriteVertexBufferCapacity`: how many sprite vertices currently fit in
  the GPU buffer.
- `m_spriteCount`, `m_spriteBatchCount`, `m_spriteDrawCallCount`, and
  `m_spriteTextureBindCount`: private frame-local counters for measuring
  batching behavior. They are reset and updated internally, but are not
  displayed or exposed through the public render API.
- `m_textures`: loaded OpenGL texture IDs and their sizes.
- `m_spriteVertices`: flat CPU-side vertex list collected during the frame.
- `m_spriteBatches`: ranges into `m_spriteVertices`, grouped by consecutive
  texture use.

OpenGL object handles are integer IDs, not C++ object pointers. For example,
textures are created with:

```cpp
glGenTextures(1, &textureId);
```

Then OpenGL uses that ID later when binding, drawing, or deleting the texture.

## Constructor Flow

The OpenGL backend constructor does the setup that must happen before drawing.

### 1. Create The OpenGL Context

```cpp
m_context = SDL_GL_CreateContext(m_window);
```

An OpenGL context is the GPU state environment used by OpenGL calls. Most
OpenGL functions cannot be used before this exists.

### 2. Make The Context Current

```cpp
SDL_GL_MakeCurrent(m_window, m_context);
```

OpenGL functions operate on the current context. This call says: from now on,
OpenGL calls should affect this window/context pair.

### 3. Load OpenGL Functions With GLAD

```cpp
auto loadOpenGLFunction = [](const char* name) -> void* {
    return reinterpret_cast<void*>(SDL_GL_GetProcAddress(name));
};
if (!gladLoadGLLoader(loadOpenGLFunction)) {
    SDL_GL_DestroyContext(m_context);
    m_context = nullptr;
    throw std::runtime_error("Failed to initialize GLAD");
}
```

GLAD loads function pointers for OpenGL functions such as `glCreateShader`,
`glGenBuffers`, `glVertexAttribPointer`, and `glDrawArrays`.

This must happen after the context exists.

After GLAD loads, the constructor prints the OpenGL version:

```cpp
const GLubyte* version = glGetString(GL_VERSION);
std::cout << "OpenGL version: " << ... << std::endl;
```

### 4. Enable Vsync And Viewport

```cpp
SDL_GL_SetSwapInterval(1);
SDL_GetWindowSize(m_window, &m_width, &m_height);
glViewport(0, 0, m_width, m_height);
```

`SDL_GL_SetSwapInterval(1)` enables vsync.

`glViewport` maps OpenGL clip-space output to window pixels.

### 5. Enable Alpha Blending

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

This is necessary for PNG transparency and normal sprite alpha blending.

### 6. Create Sprite Renderer Resources

```cpp
createSpriteRenderer();
```

This creates the shader program, VAO, and VBO used by `drawSprite`.

## Frame Flow

`beginFrame` resets frame-local batch data and clears the screen:

```cpp
m_spriteBatches.clear();
m_spriteVertices.clear();
m_spriteCount = 0;
m_spriteBatchCount = 0;
m_spriteDrawCallCount = 0;
m_spriteTextureBindCount = 0;

glClearColor(r, g, b, a);
glClear(GL_COLOR_BUFFER_BIT);
```

The `Color` type stores bytes from `0` to `255`, while OpenGL expects floats
from `0.0f` to `1.0f`, so the backend divides by `255.0f`.

`endFrame` flushes all queued sprite batches and then presents the back buffer:

```cpp
flushSpriteBatches();
SDL_GL_SwapWindow(m_window);
```

This is the OpenGL equivalent of `SDL_RenderPresent`.

## Sprite Shader

`createSpriteRenderer` creates a simple textured sprite shader.

The vertex shader receives:

- position in OpenGL clip space
- texture coordinate

```glsl
#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoord;
out vec2 fragmentTexCoord;

void main()
{
    fragmentTexCoord = texCoord;
    gl_Position = vec4(position, 0.0, 1.0);
}
```

The fragment shader samples the active texture:

```glsl
#version 330 core
in vec2 fragmentTexCoord;
uniform sampler2D spriteTexture;
out vec4 fragColor;

void main()
{
    fragColor = texture(spriteTexture, fragmentTexCoord);
}
```

The sampler uniform is set to texture unit `0`:

```cpp
glUseProgram(m_spriteProgram);
glUniform1i(glGetUniformLocation(m_spriteProgram, "spriteTexture"), 0);
glUseProgram(0);
```

That matches this draw-time code:

```cpp
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, texture.id);
```

## VAO And VBO Layout

Each sprite vertex has four floats:

```text
x, y, u, v
```

Where:

- `x, y` are OpenGL clip-space position
- `u, v` are texture coordinates

The VBO is created once with an initial capacity of 1024 `SpriteVertex`
elements. It is then grown by doubling whenever a batch needs more room:

```cpp
constexpr size_t initialSpriteVertexCapacity = 1024;
m_spriteVertexBufferCapacity = initialSpriteVertexCapacity;
m_spriteVertices.reserve(initialSpriteVertexCapacity);
m_spriteBatches.reserve(256);
glBufferData(
    GL_ARRAY_BUFFER,
    m_spriteVertexBufferCapacity * sizeof(SpriteVertex),
    nullptr,
    GL_DYNAMIC_DRAW
);
```

There are six vertices because each sprite quad is two triangles:

```text
triangle 1: top-left, bottom-left, bottom-right
triangle 2: top-left, bottom-right, top-right
```

The VAO describes the vertex format:

```cpp
glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
glEnableVertexAttribArray(0);

glVertexAttribPointer(
    1,
    2,
    GL_FLOAT,
    GL_FALSE,
    4 * sizeof(float),
    reinterpret_cast<void*>(2 * sizeof(float))
);
glEnableVertexAttribArray(1);
```

Attribute `0` maps to shader `position`.

Attribute `1` maps to shader `texCoord`.

## Texture Loading

`loadTexture` loads an image and uploads it to OpenGL:

1. `IMG_Load` loads the file into an SDL surface.
2. `SDL_ConvertSurface(..., SDL_PIXELFORMAT_RGBA32)` converts it to RGBA.
3. `SDL_LockSurface` makes the pixel memory safe to read.
4. `glGenTextures` creates an OpenGL texture ID.
5. `glTexParameteri` sets nearest filtering and clamp wrapping.
6. `glTexImage2D` uploads the pixels to the GPU.
7. The backend stores `{textureId, TextureSize}` in `m_textures`.

The upload call is:

```cpp
glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA8,
    convertedSurface->w,
    convertedSurface->h,
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    convertedSurface->pixels
);
```

Nearest filtering is used for pixel art:

```cpp
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
```

Clamp wrapping is used to avoid repeated sampling around texture edges:

```cpp
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
```

This is why texture creation belongs in the render backend:

- SDL mode creates `SDL_Texture*`.
- OpenGL mode creates OpenGL texture IDs.

Both may load the same files, but the GPU object type is renderer-specific.

## Batching Model

The public `RenderBackend` API is unchanged. Scene code still calls
`drawSprite` once per sprite.

The OpenGL backend no longer draws immediately. Instead:

- `beginFrame` clears `m_spriteBatches` and `m_spriteVertices`.
- `drawSprite` appends six vertices to `m_spriteVertices`.
- `endFrame` uploads each batch to the GPU and calls `glDrawArrays`.

The current batching rule is conservative:

- if the next sprite uses the same texture as the previous sprite, append it to
  the same batch
- if the texture changes, start a new batch

This preserves the existing draw order while reducing repeated shader binds,
VAO binds, VBO binds, and many tiny per-sprite GL calls.

It does not yet sort by texture, because sorting could change visual layering.
So if sprites alternate textures frequently, the backend still creates many
batches. A texture atlas is the next big step toward fewer draw calls.

## drawSprite Flow

`drawSprite` receives the same command the SDL backend receives:

```cpp
SpriteDrawCommand{
    texture,
    src,
    dst,
    angle
}
```

The OpenGL backend then does five things.

### 1. Look Up The OpenGL Texture

```cpp
const OpenGLTexture& texture = getTexture(command.texture);
```

This gives access to the OpenGL texture ID and texture size.

### 2. Convert Source Rect To UV Coordinates

SDL uses source rectangles in pixels:

```text
src.x, src.y, src.w, src.h
```

OpenGL texture sampling uses normalized coordinates:

```text
u = 0.0 left, 1.0 right
v = 0.0 top,  1.0 bottom
```

The backend converts like this:

```cpp
srcLeft = src.x / textureWidth;
srcRight = (src.x + src.w) / textureWidth;
srcTop = src.y / textureHeight;
srcBottom = (src.y + src.h) / textureHeight;
```

This lets sprite sheets work the same way as in the SDL backend.

### 3. Convert Destination Rect To Rotated Corners

The game gives destination rectangles in screen pixels, with the origin at the
top-left. The backend finds the center and rotates each corner around it:

```cpp
centerX = dst.x + dst.w * 0.5f;
centerY = dst.y + dst.h * 0.5f;
```

The rotation uses `command.angle` in degrees, matching the SDL renderer's angle
style.

### 4. Convert Pixel Coordinates To OpenGL Clip Space

OpenGL clip space is:

```text
x = -1 left,  +1 right
y = -1 bottom, +1 top
```

The game uses:

```text
x = 0 left, increasing right
y = 0 top,  increasing down
```

The conversion is:

```cpp
clipX = pixelX / windowWidth * 2.0f - 1.0f;
clipY = 1.0f - pixelY / windowHeight * 2.0f;
```

The `1.0f - ...` part flips the y-axis so top-left game coordinates work in
OpenGL.

### 5. Append Six Vertices To The Current Batch

The backend builds this vertex array:

```text
x, y, u, v
x, y, u, v
x, y, u, v
x, y, u, v
x, y, u, v
x, y, u, v
```

Then it appends those vertices to the flat CPU vertex list and extends the
current batch:

```cpp
appendSpriteBatchVertices(texture.id, vertices, 6);
```

No OpenGL draw call happens in `drawSprite`.

## Batch Flush Flow

`flushSpriteBatches` does the actual OpenGL work:

```cpp
glUseProgram(m_spriteProgram);
glActiveTexture(GL_TEXTURE0);
glBindVertexArray(m_spriteVertexArray);
glBindBuffer(GL_ARRAY_BUFFER, m_spriteVertexBuffer);
```

Then for each batch:

```cpp
ensureSpriteVertexBufferCapacity(batch.vertexCount);
glBindTexture(GL_TEXTURE_2D, batch.textureId);
glBufferSubData(GL_ARRAY_BUFFER, 0, batchByteSize, m_spriteVertices.data() + batch.firstVertex);
glDrawArrays(GL_TRIANGLES, 0, batchVertexCount);
```

This means one batch produces one texture bind, one buffer upload, and one draw
call, even if it contains many sprites.

The backend tracks these internal counters while flushing:

- `m_spriteCount`: number of sprites submitted this frame
- `m_spriteBatchCount`: number of batches produced this frame
- `m_spriteDrawCallCount`: number of OpenGL sprite draw calls this frame
- `m_spriteTextureBindCount`: number of texture binds during sprite flushing

They are useful to inspect while profiling, but they are not currently printed.

The biggest remaining limitation is texture switching. If many consecutive
sprites use different textures, they still become many batches. A texture atlas
or texture arrays would reduce that further.

## Scene Layer Iteration

The scene renderer still decides sprite order. `Scene::sRenderBasic` iterates
the render layers and calls `drawSprite` in that order:

```cpp
const auto& layers = m_rendererManager.getLayers();
for (const auto& layer : layers) {
    for (const auto& e : layer) {
        ...
        drawSprite(sprite, dst, transform.angle);
    }
}
```

`RendererManager::getLayers()` now returns a const reference:

```cpp
const std::vector<std::vector<EntityID>>& getLayers() const;
```

That avoids copying the full layer/entity structure every frame. This is
separate from OpenGL batching, but it helps the CPU side of rendering.

## Resize Handling

When the game changes resolution, it calls:

```cpp
m_renderBackend->onWindowResized(width, height);
```

The OpenGL backend stores the new dimensions and updates the viewport:

```cpp
m_width = width;
m_height = height;
glViewport(0, 0, m_width, m_height);
```

Those dimensions are also used by `drawSprite` to convert pixel coordinates to
clip space.

## Cleanup

OpenGL resources must be deleted manually:

```cpp
glDeleteBuffers(1, &m_spriteVertexBuffer);
glDeleteVertexArrays(1, &m_spriteVertexArray);
glDeleteProgram(m_spriteProgram);
glDeleteTextures(1, &texture.id);
SDL_GL_DestroyContext(m_context);
```

The context is made current before deletion:

```cpp
SDL_GL_MakeCurrent(m_window, m_context);
```

That matters because OpenGL deletion calls operate on the current context.

The CPU-side batch vectors are normal C++ containers and clean themselves up
automatically when the backend is destroyed.

## Important Difference From SDL Rendering

SDL rendering usually looks like:

```cpp
SDL_RenderTextureRotated(renderer, texture, &src, &dst, angle, nullptr, SDL_FLIP_NONE);
```

You pass SDL the texture, source rect, destination rect, and angle. SDL hides the
GPU details.

OpenGL rendering is more explicit:

1. create shader program
2. create texture objects
3. create vertex buffers
4. convert positions to clip space
5. convert source pixels to UV coordinates
6. bind the shader
7. bind the texture
8. bind the VAO/VBO
9. draw triangles

The OpenGL backend is doing those steps manually now.

## Next Useful Steps

Good next improvements:

- implement `drawRect` and `fillRect` with a color shader
- implement text rendering, likely by rasterizing TTF text to an OpenGL texture
- add a projection matrix so `drawSprite` can use pixel coordinates more cleanly
- group more sprites by atlas/texture so batches get larger
- expose or log the private frame counters while profiling, so sprite count,
  batch count, draw calls, and texture binds can be compared between scenes
- add error/debug logging around OpenGL setup and shader compilation

The current implementation is intentionally direct and easy to read. Once it is
visually correct, it can be optimized.
