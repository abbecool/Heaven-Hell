# Heaven-Hell
My first attempt at creating a C++ game from scratch
-----------------------------------------------------
In this game you control two players, an angel and the devil. 
The objective of the game is to get both players to their respective goals/gates.
The catch is that the angel and devil are controlled by the same keys on the keyboard.
Get both to their goals without them getting stuck or dying.

# Heaven-Hell 2
This version of the game is a top-down RPG 
-----------------------------------------------------
Here you play as a XXX who is in charge of deciding who gets to go to heaven and who goes to hell (Skärselden)
You dont know you origins and it is a mystery. There has been a greate battle and the starting area is in ruins.
Your quest is to find out about you origin/backstory.

Add trees, chop down a tree leaving a tree stump, cast a spell/potion on the tree stump to turn it into a rooter!
Spells primaraly deal damage and Potions primaraly inflict status effects.

Add the ability climb trees to see further

# Developer Setup
Install CMake (Windows x64 Installer): https://cmake.org/download/

Download and extract relevant SDL2 dependancies to sibling folder named "SDL2" and extract VC download inside.

## SDL2 Downloads

| SDL version            | File to download | Link |
|------------------------|:----------------:|:------------------:|
| SDL2              | SDL2-devel-2.32.8-VC.zip  | https://github.com/libsdl-org/SDL/releases       |
| SDL2_image    | SDL2_image-devel-2.8.8-VC.zip     | https://github.com/libsdl-org/SDL_image/releases  |
| SDL2_ttf         | SDL2_ttf-devel-2.24.0-VC.zip      | https://github.com/libsdl-org/SDL_ttf/releases       |
| SDL2_mixer              | SDL2_mixer-devel-2.8.1-VC.zip      | https://github.com/libsdl-org/SDL_mixer/releases  |

The SDL2 folder should look like this.![](SDL2_folder.png)

cmake -S . -D CMAKE_PREFIX_PATH="C:\\projects\\privat\\SDL2\\SDL2-2.32.8;C:\\projects\\privat\\SDL2\\SDL2_image-2.8.8;C:\\projects\\privat\\SDL2\\SDL2_mixer-2.8.1;C:\\projects\\privat\\SDL2\\SDL2_ttf-2.24.0" -B build/

cmake --build build --config release

cmake --build build --config debug