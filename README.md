# Heaven-Hell

Heaven-Hell is currently a top-down RPG built in C++20 with SDL3 as the
platform layer. The renderer is abstracted behind `RenderBackend`; the project
has both an SDL renderer and an OpenGL renderer, and the game currently defaults
to OpenGL in `Game.hpp`.

## Current Architecture

- `src/core`: game loop, SDL platform wrapper, input translation, shared pixel
  image loading.
- `src/render`: renderer-neutral types plus SDL and OpenGL backend
  implementations.
- `src/assets`: asset catalog and sprite metadata. Texture and font resources
  are loaded through `RenderBackend`.
- `src/ecs`: ECS storage, entities, components, and component factory.
- `src/scenes`: game-specific scenes and gameplay flow.
- `src/physics`: math, camera, level loading, quadtree, collision, and movement
  helpers.
- `src/story`: story, quest, and event support.

The SDL platform layer owns SDL initialization, the window, input polling,
audio loading/playback, and CPU-side image pixel loading. Rendering resources
belong to the active render backend.

## Developer Setup

### Windows Setup with MSYS2

Install MSYS2 from:

```text
https://github.com/msys2/msys2-installer/releases
```

Use the **MSYS2 UCRT64** terminal. This project is set up for the UCRT64 MinGW
toolchain.

Update MSYS2:

```sh
pacman -Syu
```

If MSYS2 asks you to close the terminal after updating, close it, open
**MSYS2 UCRT64** again, and run:

```sh
pacman -Syu
```

Install the compiler, CMake, Ninja, and debugger:

```sh
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-gdb
```

Install SDL3 and the SDL3 extension libraries:

```sh
pacman -S mingw-w64-ucrt-x86_64-sdl3 mingw-w64-ucrt-x86_64-sdl3-image mingw-w64-ucrt-x86_64-sdl3-mixer mingw-w64-ucrt-x86_64-sdl3-ttf
```

These packages install Windows tools and libraries into:

```text
C:/msys64/ucrt64
```

The main tools should be:

```text
C:/msys64/ucrt64/bin/g++.exe
C:/msys64/ucrt64/bin/cmake.exe
C:/msys64/ucrt64/bin/ninja.exe
C:/msys64/ucrt64/bin/gdb.exe
```

When MSYS2 is ready, open the repository in VS Code, select a launch
configuration, and press F5. VS Code will configure CMake, build the project,
and run the game.

## Windows Manual Build

The VS Code tasks and package script use CMake presets. Windows presets use
Ninja build trees under `build/Windows/Ninja`.

PowerShell:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
```

Debug:

```powershell
C:/msys64/ucrt64/bin/cmake.exe --preset windows-ninja-debug
C:/msys64/ucrt64/bin/cmake.exe --build --preset windows-ninja-debug
```

Release:

```powershell
C:/msys64/ucrt64/bin/cmake.exe --preset windows-ninja-release
C:/msys64/ucrt64/bin/cmake.exe --build --preset windows-ninja-release
```

The Windows executable is created at:

```text
run/Debug/heavenhell.exe
run/Release/heavenhell.exe
```

## Windows Release Package

To create a Windows release zip, run the package script from PowerShell:

```powershell
.\scripts\package-release.ps1
```

By default, the script uses the MSYS2 UCRT64 tools from
`C:/msys64/ucrt64/bin`, builds the `Release` configuration, copies the game
executable, assets, config files, and runtime DLL dependencies, then creates:

```text
dist/HeavenHell-<version>-win64.zip
```

The package version comes from `version.txt`.

If MSYS2 is installed somewhere else, pass the MinGW/UCRT64 `bin` directory:

```powershell
.\scripts\package-release.ps1 -MingwBin "D:/msys64/ucrt64/bin"
```

To package an existing build without rebuilding first:

```powershell
.\scripts\package-release.ps1 -SkipBuild
```

## Tests

Configure and build the debug preset, then run CTest:

```powershell
C:/msys64/ucrt64/bin/cmake.exe --preset windows-ninja-debug
C:/msys64/ucrt64/bin/cmake.exe --build --preset windows-ninja-debug
C:/msys64/ucrt64/bin/ctest.exe --preset windows-ninja-debug --output-on-failure
```

## Linux Setup

Linux support exists in the CMake and VS Code setup, but the current day-to-day
workflow is Windows/MSYS2/Ninja.

Install a C++ compiler, CMake, GDB, and the SDL3 development packages.

On Arch Linux:

```sh
sudo pacman -S gcc cmake gdb sdl3 sdl3_image sdl3_mixer sdl3_ttf
```

On Debian/Ubuntu, package names may vary depending on distro version:

```sh
sudo apt install build-essential cmake gdb libsdl3-dev libsdl3-image-dev libsdl3-mixer-dev libsdl3-ttf-dev
```

Manual build:

```sh
cmake --preset linux-debug
cmake --build --preset linux-debug
```

```sh
cmake --preset linux-release
cmake --build --preset linux-release
```

The Linux executable is created at:

```text
run/Debug/heavenhell
run/Release/heavenhell
```

## Current Backend Notes

- SDL3 is still the platform API.
- OpenGL is the current default render driver.
- The SDL backend is useful as a compatibility/reference renderer.
- The OpenGL backend supports batched textured sprites, filled/drawn
  rectangles, and text through glyph atlases.
- World-space camera projection has not moved into the renderer yet; scenes
  still calculate camera-to-screen transforms on the CPU.
- CTest is enabled with small `Vec2`, `RandomArray`, `SpriteDefinition`, and
  ECS/component-pool test suites.

See [doc/BackendSetupRoadmap.md](doc/BackendSetupRoadmap.md) for the recommended
backend/setup work before shifting fully back to gameplay features.
