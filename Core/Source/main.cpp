#include <SDL2/SDL.h>
#include "Game.h"

// Top part is heaven, bot part is hell. arrows control both.
// add function to switch places at certain points. AND OR invert colors to avoid obsticals / fly over or 
// add obsticles that GAME OVER when touch.
// add obsticles that can be touched in order to move one rect but not the other.
// make key for one of the players that is needed to unlock other player and make it movable

// Alternative idea for game. RPG like with different weapons and upgrades. zelda/pokemon still with quests and side-quests! 
// Fundemental idea is the same, during nighttime player sleepwalks in a dreamworld with different world and enemies. Either both day and night world are visible at the same time, 
// or night world only at night, or day and night world overlapped during night. some dreams are nightmares and some are plesant.
// OR
// Different elemental types like, ice, fire, earth, air etc. each with special damage multipliers and resistances on enemies.
// Different weapons have differant elemental effects and damage types... very original

int main(int argc, char* argv[]){   
    Game g("config_files/assets.txt", "config_files/text.txt");
    g.run();

    return EXIT_SUCCESS;
}