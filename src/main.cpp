#include <SDL.h>
#include <SDL_main.h>

#include "core/Game.h"
#include "external/json.hpp"
using json = nlohmann::json;

// Alternative idea for game. RPG like with different weapons and upgrades. zelda/pokemon still with quests and side-quests! 
// Fundemental idea is the same, during nighttime player sleepwalks in a dreamworld with different world and enemies. Either both day and night world are visible at the same time, 
// or night world only at night, or day and night world overlapped during night. some dreams are nightmares and some are plesant.
// OR
// Different elemental types like, ice, fire, earth, air etc. each with special damage multipliers and resistances on enemies.
// Different weapons have differant elemental effects and damage types... very original

int main(int argc, char* argv[]){   
    Game g("config_files/assets.json", "config_files/text.txt");
    g.run();

    return EXIT_SUCCESS;
}