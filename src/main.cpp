#include "core/Game.hpp"
#include "external/json.hpp"
using json = nlohmann::json;

// Alternative idea for game. RPG like with different weapons and upgrades. zelda/pokemon still with quests and side-quests! 
// Different elemental types like, ice, fire, earth, air etc. each with special damage multipliers and resistances on enemies.
// Different weapons have differant elemental effects and damage types... very original

int main(int argc, char* argv[]){   
    Game g("config_files/assets.json", "config_files/text.txt");
    g.run();

    return EXIT_SUCCESS;
}