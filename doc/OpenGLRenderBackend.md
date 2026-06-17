# OpenGLRenderBackend Current State

This document explains how OpenGL is currently set up in the project, what each
piece does, and how it differs from the existing SDL renderer. It is written as
a learning note for continuing work on `OpenGLRenderBackend`.

At the moment, the OpenGL renderer is a smoke-test renderer. It proves that the
game can:

- create an SDL window that supports OpenGL
- create an OpenGL context
- load OpenGL functions through GLAD
- compile and link shaders
- create a VAO and VBO
- draw a triangle
- load image files into OpenGL texture objects
- clear and present frames without crashing

It does not yet render game sprites, rectangles, UI text, or fonts.

## Big Picture

The project now has two render backend implementations behind the same
`RenderBackend` interface:

- `SDLRenderBackend`: uses `SDL_Renderer` and `SDL_Texture`.
- `OpenGLRenderBackend`: uses an SDL-created OpenGL context and OpenGL objects.

The game code talks to `RenderBackend`, not directly to SDL rendering or
OpenGL. This is the useful abstraction:

```cpp
m_renderBackend->beginFrame({0, 0, 0, 255});
update();
sUserInput();
FrametimeHandler();
m_renderBackend->endFrame();
```

In SDL mode, those calls eventually become `SDL_RenderClear`,
`SDL_RenderTextureRotated`, and `SDL_RenderPresent`.

In OpenGL mode, they currently become `glClearColor`, `glClear`,
`glDrawArrays`, and `SDL_GL_SwapWindow`.

## Build Setup

The OpenGL function loader is GLAD. It lives in:

```text
src/external/glad/include/glad/glad.h
src/external/glad/src/glad.c
```

`CMakeLists.txt` builds GLAD as a static library:

```cmake
set_source_files_properties(src/external/glad/src/glad.c PROPERTIES LANGUAGE CXX)
add_library(glad STATIC
    src/external/glad/src/glad.c
)

target_include_directories(glad PUBLIC
    src/external/glad/include
)
```

The game executable links against `glad`:

```cmake
target_link_libraries(heavenhell PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_mixer::SDL3_mixer
    SDL3_ttf::SDL3_ttf
    glad
)
```

Why GLAD is needed:

OpenGL functions like `glGenBuffers`, `glBindVertexArray`, and `glCreateShader`
are not normal functions you can reliably call directly on all platforms.
GLAD loads function pointers for the OpenGL version and driver that are active
at runtime. That loading can only happen after an OpenGL context exists.

## SDLPlatform's Role

`SDLPlatform` still owns SDL initialization, the window, input, audio, and
generic CPU-side image pixel loading.

When the selected render driver is OpenGL, `SDLPlatform` creates the window
with OpenGL support:

```cpp
SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE;
if (renderDriver == RenderDriver::OpenGL) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    windowFlags = static_cast<SDL_WindowFlags>(windowFlags | SDL_WINDOW_OPENGL);
}

m_window = SDL_CreateWindow(title, width, height, windowFlags);
```

Important details:

- `SDL_WINDOW_OPENGL` means the window may be used with an OpenGL context.
- `SDL_GL_CONTEXT_MAJOR_VERSION` and `SDL_GL_CONTEXT_MINOR_VERSION` request
  OpenGL 3.3.
- `SDL_GL_CONTEXT_PROFILE_CORE` requests the modern core profile.

This does not create the OpenGL context yet. It only creates a window that is
allowed to have one.

## Game Factory

`Game.cpp` chooses which render backend to create:

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

This means both renderers receive the same `SDL_Window*`, but they use it very
differently.

The SDL renderer passes the window into `SDL_CreateRenderer`.

The OpenGL renderer passes the window into `SDL_GL_CreateContext`.

## What OpenGLRenderBackend Owns

`OpenGLRenderBackend` currently owns:

```cpp
SDL_Window* m_window = nullptr;
SDL_GLContext m_context = nullptr;
int m_width = 1;
int m_height = 1;
unsigned int m_triangleProgram = 0;
unsigned int m_triangleVertexArray = 0;
unsigned int m_triangleVertexBuffer = 0;
std::map<std::string, OpenGLTexture> m_textures;
```

The important OpenGL objects are:

- `m_context`: the active OpenGL state machine for this window.
- `m_triangleProgram`: the linked shader program used to draw the debug
  triangle.
- `m_triangleVertexArray`: the VAO, which stores how vertex data is interpreted.
- `m_triangleVertexBuffer`: the VBO, which stores the triangle's vertex data on
  the GPU.
- `m_textures`: OpenGL texture IDs and their sizes.

OpenGL object handles are unsigned integers. For example, a texture is not a
pointer like `SDL_Texture*`; it is a numeric ID created by OpenGL:

```cpp
unsigned int textureId = 0;
glGenTextures(1, &textureId);
```

The renderer stores those IDs and later gives them back to OpenGL when it wants
to use or delete them.

## Constructor Flow

The constructor is where the OpenGL renderer becomes usable.

### 1. Create an OpenGL context

```cpp
m_context = SDL_GL_CreateContext(m_window);
```

An OpenGL context is the thing that stores OpenGL state and gives access to the
GPU through OpenGL calls. You cannot call most OpenGL functions before this
exists.

### 2. Make the context current

```cpp
SDL_GL_MakeCurrent(m_window, m_context);
```

OpenGL uses the idea of a current context. When you call `glClear`,
`glCreateShader`, or `glGenTextures`, those calls affect the current context.

If the context is not current, OpenGL calls may fail or affect a different
context.

### 3. Load OpenGL functions with GLAD

```cpp
auto loadOpenGLFunction = [](const char* name) -> void* {
    return reinterpret_cast<void*>(SDL_GL_GetProcAddress(name));
};
if (!gladLoadGLLoader(loadOpenGLFunction)) {
    throw std::runtime_error("Failed to initialize GLAD");
}
```

`SDL_GL_GetProcAddress` asks SDL for the address of an OpenGL function.
GLAD calls this repeatedly for all OpenGL functions it knows about.

After this succeeds, functions like `glCreateShader`, `glGenBuffers`,
`glVertexAttribPointer`, and `glDrawArrays` are ready to use.

### 4. Enable vsync

```cpp
SDL_GL_SetSwapInterval(1);
```

This tells SDL/OpenGL to wait for the display refresh when swapping buffers.
That usually prevents tearing and avoids rendering as fast as possible.

### 5. Initialize viewport size

```cpp
SDL_GetWindowSize(m_window, &m_width, &m_height);
glViewport(0, 0, m_width, m_height);
```

`glViewport` maps OpenGL normalized device coordinates to actual window pixels.
If the viewport is wrong, rendering may be stretched, clipped, or invisible.

### 6. Print OpenGL version

```cpp
const GLubyte* version = glGetString(GL_VERSION);
std::cout << "OpenGL version: " << ... << std::endl;
```

This is a useful verification step. If it prints a version, the context and
GLAD setup got far enough to query the driver.

### 7. Create the debug triangle

```cpp
createDebugTriangle();
```

This creates all GPU state needed for the visible test triangle.

## Frame Flow

`Game::run()` calls this each frame:

```cpp
m_renderBackend->beginFrame({0, 0, 0, 255});
...
m_renderBackend->endFrame();
```

In OpenGL mode, `beginFrame` clears the screen:

```cpp
void OpenGLRenderBackend::beginFrame(Color clearColor)
{
    glViewport(0, 0, m_width, m_height);
    glClearColor(
        static_cast<float>(clearColor.r) / 255.0f,
        static_cast<float>(clearColor.g) / 255.0f,
        static_cast<float>(clearColor.b) / 255.0f,
        static_cast<float>(clearColor.a) / 255.0f
    );
    glClear(GL_COLOR_BUFFER_BIT);
}
```

SDL colors use bytes from `0` to `255`. OpenGL clear color uses floats from
`0.0f` to `1.0f`, so the backend divides by `255.0f`.

`endFrame` currently draws the triangle and presents the frame:

```cpp
void OpenGLRenderBackend::endFrame()
{
    glUseProgram(m_triangleProgram);
    glBindVertexArray(m_triangleVertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glUseProgram(0);

    SDL_GL_SwapWindow(m_window);
}
```

`SDL_GL_SwapWindow` is the OpenGL equivalent of `SDL_RenderPresent` here.
The app renders into a back buffer, then swaps that back buffer onto the screen.

## The Debug Triangle

The triangle is made in `createDebugTriangle`.

It exists only to verify that OpenGL rendering works beyond a blank clear. It is
not part of the game renderer yet.

### Shaders

Modern OpenGL requires shaders. A shader is a small GPU program.

The current renderer has two shaders:

- Vertex shader: runs once per vertex.
- Fragment shader: runs once per pixel/fragment covered by the triangle.

The vertex shader:

```glsl
#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec3 color;
out vec3 vertexColor;

void main()
{
    vertexColor = color;
    gl_Position = vec4(position, 0.0, 1.0);
}
```

This receives:

- `position`: a 2D vertex position.
- `color`: a per-vertex RGB color.

It outputs:

- `gl_Position`: the final clip-space position.
- `vertexColor`: passed to the fragment shader.

The fragment shader:

```glsl
#version 330 core
in vec3 vertexColor;
out vec4 fragColor;

void main()
{
    fragColor = vec4(vertexColor, 1.0);
}
```

This receives interpolated color from the vertex shader and outputs the final
pixel color.

### Shader compile and link

The helpers at the top of `OpenGLRenderBackend.cpp` do two steps:

1. `compileShader`: turns one GLSL source string into a GPU shader object.
2. `createShaderProgram`: links the vertex and fragment shaders into one
   program.

The program is what gets used during drawing:

```cpp
glUseProgram(m_triangleProgram);
```

### Vertex data

The triangle vertex data is:

```cpp
const float vertices[] = {
     0.0f,  0.6f,  1.0f, 0.2f, 0.2f,
    -0.6f, -0.5f,  0.2f, 1.0f, 0.2f,
     0.6f, -0.5f,  0.2f, 0.4f, 1.0f
};
```

Each vertex has five floats:

```text
x, y, r, g, b
```

So the first vertex is:

```text
position = 0.0, 0.6
color    = 1.0, 0.2, 0.2
```

The positions are in normalized device coordinates. In this coordinate system:

```text
x = -1.0 is left edge
x =  1.0 is right edge
y = -1.0 is bottom edge
y =  1.0 is top edge
```

That is why `0.0, 0.6` puts the top triangle point above the center.

This is different from the game's normal 2D coordinates, where positions are in
pixels or virtual game units. A real sprite renderer will need to convert game
rectangles into OpenGL clip-space coordinates, usually with a projection matrix.

### VBO

The VBO stores vertex data on the GPU:

```cpp
glGenBuffers(1, &m_triangleVertexBuffer);
glBindBuffer(GL_ARRAY_BUFFER, m_triangleVertexBuffer);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
```

`GL_STATIC_DRAW` means the data is expected to be uploaded once and drawn many
times.

### VAO

The VAO stores the vertex layout:

```cpp
glGenVertexArrays(1, &m_triangleVertexArray);
glBindVertexArray(m_triangleVertexArray);
```

Then the backend describes how to read each vertex:

```cpp
glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
glEnableVertexAttribArray(0);
```

Attribute `0` is the position:

- index: `0`
- components: `2`
- type: `float`
- stride: `5 * sizeof(float)`
- offset: `nullptr`, meaning start at the beginning of each vertex

Then color:

```cpp
glVertexAttribPointer(
    1,
    3,
    GL_FLOAT,
    GL_FALSE,
    5 * sizeof(float),
    reinterpret_cast<void*>(2 * sizeof(float))
);
glEnableVertexAttribArray(1);
```

Attribute `1` is the color:

- index: `1`
- components: `3`
- type: `float`
- stride: `5 * sizeof(float)`
- offset: `2 * sizeof(float)`, because color starts after x and y

This matches the shader declarations:

```glsl
layout (location = 0) in vec2 position;
layout (location = 1) in vec3 color;
```

That matching is important. If the shader says location `0` is position, the
VAO must describe location `0` as position.

## Texture Loading

`OpenGLRenderBackend::loadTexture` currently loads image files and uploads them
to OpenGL:

1. Load file with `IMG_Load`.
2. Convert the surface to `SDL_PIXELFORMAT_RGBA32`.
3. Lock the surface so raw pixel memory is safe to read.
4. Create an OpenGL texture ID.
5. Bind the texture.
6. Set texture filtering and wrapping.
7. Upload pixels with `glTexImage2D`.
8. Store the OpenGL texture ID and size.
9. Destroy the temporary SDL surface.

The key OpenGL upload call is:

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

The important parts are:

- `GL_TEXTURE_2D`: this is a normal 2D image texture.
- `GL_RGBA8`: store it on the GPU as 8-bit RGBA.
- `convertedSurface->w`, `convertedSurface->h`: texture dimensions.
- `GL_RGBA`, `GL_UNSIGNED_BYTE`: format of the incoming CPU pixel data.
- `convertedSurface->pixels`: pointer to CPU-side pixel data.

Texture filtering is set to nearest:

```cpp
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
```

This is good for pixel art because it avoids blurry interpolation.

Texture wrapping is clamped:

```cpp
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
```

This prevents sampling from repeating around the texture edges.

Currently, textures are loaded but not drawn. The next real renderer step is to
make `drawSprite` bind the correct texture and draw a textured quad.

## Why Texture Loading Is In The Render Backend

The platform can load generic pixels. It already has:

```cpp
PixelImage SDLPlatform::loadImagePixels(const std::string& path) const
```

That is useful for CPU-side systems like level loading.

But GPU texture creation belongs to the renderer:

- SDL renderer creates `SDL_Texture*`.
- OpenGL renderer creates `GLuint` texture IDs.

Both start from an image file, but the final GPU object is renderer-specific.
That is why `RenderBackend::loadTexture(name, path)` remains the right current
interface.

Later, if you want to reduce duplicated image decoding, you could change the
interface to something like:

```cpp
virtual void createTexture(const std::string& name, const PixelImage& image) = 0;
```

Then `Assets` or `SDLPlatform` could decode pixels once, and each renderer could
upload them in its own way. That is a reasonable future refactor, but not needed
for the current smoke-test stage.

## Resize Handling

`Game::updateResolution` calls:

```cpp
m_renderBackend->onWindowResized(width, height);
```

The OpenGL renderer stores the new size and updates the viewport:

```cpp
void OpenGLRenderBackend::onWindowResized(int width, int height)
{
    m_width = width;
    m_height = height;
    glViewport(0, 0, m_width, m_height);
}
```

`beginFrame` also calls `glViewport`, which makes the current viewport explicit
every frame. This is slightly redundant but harmless and useful while the
renderer is young.

## Cleanup

OpenGL does not automatically delete GPU objects when your C++ variables go out
of scope. The destructor deletes them explicitly:

```cpp
glDeleteBuffers(1, &m_triangleVertexBuffer);
glDeleteVertexArrays(1, &m_triangleVertexArray);
glDeleteProgram(m_triangleProgram);
glDeleteTextures(1, &texture.id);
SDL_GL_DestroyContext(m_context);
```

The context is made current before deletion:

```cpp
SDL_GL_MakeCurrent(m_window, m_context);
```

This matters because OpenGL deletion calls act on the current context.

## What Is Not Implemented Yet

These functions intentionally do nothing right now:

```cpp
void OpenGLRenderBackend::loadFont(...)
void OpenGLRenderBackend::drawSprite(...)
void OpenGLRenderBackend::drawRect(...)
void OpenGLRenderBackend::fillRect(...)
void OpenGLRenderBackend::drawText(...)
```

`loadFont` does nothing so asset loading does not crash.

`drawText` does nothing, so FPS text and UI text are invisible in OpenGL mode.

`drawSprite` does nothing, so game sprites are invisible in OpenGL mode.

The triangle is drawn from `endFrame`, not from the game scene system. That is
intentional for now because it is just a backend verification shape.

## OpenGL Concepts To Keep Straight

### OpenGL context

The context is the active OpenGL world. OpenGL calls operate on the current
context. SDL creates it for us with `SDL_GL_CreateContext`.

### GLAD

GLAD loads OpenGL function pointers after a context exists. Include
`<glad/glad.h>` before using OpenGL functions.

### Shader

A GPU program. The vertex shader positions geometry. The fragment shader colors
pixels.

### Shader program

A linked set of shaders used for drawing.

### VBO

Vertex Buffer Object. Stores vertex data on the GPU.

### VAO

Vertex Array Object. Stores how vertex data should be interpreted. It remembers
attribute layout calls like `glVertexAttribPointer`.

### Texture

A GPU image. In OpenGL it is represented by an unsigned integer ID.

### Binding

OpenGL often works by binding an object, then calling functions that modify or
use the currently bound object:

```cpp
glBindTexture(GL_TEXTURE_2D, textureId);
glTexParameteri(...);
glTexImage2D(...);
```

This can feel strange compared to SDL, where you pass object pointers directly
to most functions.

## Next Step: Textured Sprite Rendering

The natural next step is to make `drawSprite` work.

A simple path would be:

1. Replace the debug triangle shader with a textured quad shader.
2. Create a reusable quad VAO/VBO.
3. For each `SpriteDrawCommand`:
   - find the texture by `command.texture`
   - convert `command.src` into UV coordinates
   - convert `command.dst` into screen-space or clip-space positions
   - bind the texture
   - draw two triangles forming a rectangle
4. Add alpha blending:

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

For game-style 2D rendering, you will probably want an orthographic projection
that maps your virtual pixel coordinates to OpenGL clip space. That lets the
game keep using rectangles like:

```cpp
RectF{ x, y, w, h }
```

while OpenGL receives normalized coordinates internally.

## Current Status In One Sentence

OpenGL mode currently owns a valid SDL OpenGL context, has GLAD loaded, uploads
textures for asset metadata, clears the window, and draws a hard-coded debug
triangle, but it does not yet translate the game's normal render commands into
OpenGL draw calls.
