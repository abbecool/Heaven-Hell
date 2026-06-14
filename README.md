# Heaven-Hell
This version of the game is a top-down RPG

# Developer Setup

## Windows Setup with MSYS2

Install MSYS2 installer (C++ compiler) by clicking the link: https://github.com/msys2/msys2-installer/releases

Make sure to use the **MSYS2 UCRT64** terminal. This project is set up to use the UCRT64 MinGW compiler.

First update MSYS2:

```sh
pacman -Syu
```

If MSYS2 asks you to close the terminal after updating, close it, open **MSYS2 UCRT64** again, and run:

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

These packages install Windows libraries into:

```text
C:/msys64/ucrt64
```

The compiler should be located at:

```text
C:/msys64/ucrt64/bin/g++.exe
```

The debugger should be located at:

```text
C:/msys64/ucrt64/bin/gdb.exe
```

Ninja should be located at:

```text
C:/msys64/ucrt64/bin/ninja.exe
```

When MSYS2 is ready, open the repository in VS Code, select the desired launch configuration, and press F5. VS Code will configure CMake, build the project, and run the game.

## Windows Manual Build

To configure and build manually from the terminal:

```sh
cmake -S . -B build/Windows/Ninja/Debug -DCMAKE_BUILD_TYPE=Debug -G "Ninja"
cmake --build build/Windows/Ninja/Debug
```

```sh
cmake -S . -B build/Windows/Ninja/Release -DCMAKE_BUILD_TYPE=Release -G "Ninja"
cmake --build build/Windows/Ninja/Release
```

The Windows executable is created at:

```text
run/Debug/heavenhell.exe
run/Release/heavenhell.exe
```

## Linux Setup (only tested on Arch)

Install a C++ compiler, CMake, GDB, and the SDL3 development packages.

On Arch Linux:

```sh
sudo pacman -S gcc cmake gdb sdl3 sdl3_image sdl3_mixer sdl3_ttf
```

On Debian/Ubuntu:

```sh
sudo apt install build-essential cmake gdb libsdl3-dev libsdl3-image-dev libsdl3-mixer-dev libsdl3-ttf-dev
```

Open the repository in VS Code, select either `Run HeavenHell (Arch Release)` or `Debug HeavenHell (Arch Debug)`, and press F5. VS Code will configure CMake, build the project, and run the game.

To configure and build manually from the terminal:

```sh
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug
```

```sh
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release
```

The Linux executable is created at:

```text
run/Debug/heavenhell
run/Release/heavenhell
```
