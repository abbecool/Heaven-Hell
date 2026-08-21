#pragma once

#include "ecs/Components.hpp"

#include <array>
#include <cstddef>

inline constexpr std::size_t FactionCount =
    static_cast<std::size_t>(Faction::Neutral) + 1;

using FactionRelationMatrix =
    std::array<std::array<bool, FactionCount>, FactionCount>;

inline constexpr FactionRelationMatrix FactionRelations = {{
//  Demon,  Enemy, Wizard,Dwarf, Elf,   Knight,Neutral
    {false, true,  true,  true,  true,  true,  false}, // Demon
    {true,  false, false, false, false, false, false}, // Enemy
    {true,  true,  false, true,  true,  true,  false}, // Wizard
    {true,  true,  true,  false, true,  true,  false}, // Dwarf
    {true,  true,  true,  true,  false, true,  false}, // Elf
    {true,  true,  true,  true,  true,  false, false}, // Knight
    {true,  false, false, false, false, false, false}, // Neutral
}};

[[nodiscard]] constexpr bool isHostile(Faction attacker, Faction defender)
{
    return FactionRelations[static_cast<std::size_t>(attacker)]
                           [static_cast<std::size_t>(defender)];
}