#include <vector>
#include <string>

// Vector of Damage Types
std::vector<std::string> damageTypes = {
    "Fire",
    "Water",
    "Rock",
    "Air",
    "Ice",
    "Lightning",
    "Grass",

    "Piercing",
    "Slashing",
    "Blunt",
    "Explosive",

    "Magic",
    "Light",
    "Dark",
};

// Vector of Enemy Types
std::vector<std::string> enemyTypes = {
    "Fire",
    "Water",
    "Rock",
    "Air",
    "Ice",
    "Lightning",
    "Grass",

    "Armored",

    "Organic",
    "Flesh",
    "Undead",

    "Light",
    "Dark",
};

#include <unordered_map>
#include <unordered_set>
#include <string>

// Effective Damage to Enemy Type Map
std::unordered_map<std::string, std::unordered_set<std::string>> effectiveDamageToEnemyMap = {
    {"Fire",        {"Ice", "Grass", }},
    {"Water",       {"Fire", "Rock"}},
    {"Lightning",   {"Water", "Air"}},
    {"Ice",         {"Grass", "Air"}},
    {"Rock",        {"Ice", "Lightning"}},
    {"Air",         {"Air"}},

    {"Piercing",    {"Armored", "Flesh"}},
    {"Slashing",    {"Flesh", "Organic"}},
    {"Blunt",       {"Armored"}},
    {"Explosive",   {"Armored"}},

    // {"Magic",       {"Undead", "Dark"}},
    {"Light",       {"Dark"}},
    {"Dark",        {"Light"}}
};

// Ineffective Damage to Enemy Type Map
std::unordered_map<std::string, std::unordered_set<std::string>> ineffectiveDamageToEnemyMap = {
    {"Fire",        {"Water", "Rock", }},
    {"Water",       {"Ice", "Grass"}},
    {"Lightning",   {"Rock", "Fire"}},
    {"Ice",         {"Grass", "Air"}},
    {"Rock",        {"Water", "Rock"}},
    {"Air",         {"Rock", "Lightning"}},

    {"Piercing",    {}},
    {"Slashing",    {"Armored"}},
    {"Blunt",       {}},
    {"Explosive",   {}},

    // {"Magic",       {"Undead", "Dark"}},
    {"Light",       {"Light"}},
    {"Dark",        {"Dark"}}
};

