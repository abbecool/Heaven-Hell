# Heaven-Hell
This version of the game is a top-down RPG
-----------------------------------------------------
Here you play as a XXX who is in charge of deciding who gets to go to heaven and who goes to hell (Skärselden)
You dont know you origins and it is a mystery. There has been a great battle and the starting area is in ruins.
Your quest is to find out about you origin/backstory.

Add trees, chop down a tree leaving a tree stump, cast a spell/potion on the tree stump to turn it into a rooter!
Spells primaraly deal damage and Potions primaraly inflict status effects.

Add the ability climb trees to see further

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

Install SDL2 and the SDL2 extension libraries:

```sh
pacman -S mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_image mingw-w64-ucrt-x86_64-SDL2_mixer mingw-w64-ucrt-x86_64-SDL2_ttf
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

This project does not need a sibling SDL2 folder when using MSYS2 packages.

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

Install a C++ compiler, CMake, GDB, and the SDL2 development packages.

On Arch Linux:

```sh
sudo pacman -S gcc cmake gdb sdl2 sdl2_image sdl2_mixer sdl2_ttf
```

On Debian/Ubuntu:

```sh
sudo apt install build-essential cmake gdb libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
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
