#pragma once

#include "ecs/Components.hpp"

// A faction's view of every other faction. `true` means hostile.
struct FactionRelations
{
    bool demon = false;
    bool enemy = false;
    bool wizard = false;
    bool dwarf = false;
    bool elf = false;
    bool knight = false;
    bool neutral = false;

    [[nodiscard]] constexpr bool relationTo(Faction faction) const
    {
        switch (faction) {
        case Faction::Demon:   return demon;
        case Faction::Enemy:   return enemy;
        case Faction::Wizard:  return wizard;
        case Faction::Dwarf:   return dwarf;
        case Faction::Elf:     return elf;
        case Faction::Knight:  return knight;
        case Faction::Neutral: return neutral;
        }

        return false;
    }
};

inline constexpr FactionRelations DemonHostileRelations{
    .demon = false,
    .enemy = false,
    .wizard = false,
    .dwarf = false,
    .elf = false,
    .knight = false,
    .neutral = false,
};

inline constexpr FactionRelations EnemyHostileRelations{
    .demon = true,
    .enemy = false,
    .wizard = true,
    .dwarf = true,
    .elf = true,
    .knight = true,
    .neutral = false,
};

inline constexpr FactionRelations WizardHostileRelations{
    .demon = true,
    .enemy = false,
    .wizard = false,
    .dwarf = false,
    .elf = false,
    .knight = false,
    .neutral = false,
};

inline constexpr FactionRelations DwarfHostileRelations{
    .demon = true,
    .enemy = true,
    .wizard = false,
    .dwarf = false,
    .elf = false,
    .knight = false,
    .neutral = false,
};

inline constexpr FactionRelations ElfHostileRelations{
    .demon = true,
    .enemy = true,
    .wizard = false,
    .dwarf = false,
    .elf = false,
    .knight = true,
    .neutral = false,
};

inline constexpr FactionRelations KnightHostileRelations{
    .demon = true,
    .enemy = true,
    .wizard = false,
    .dwarf = false,
    .elf = true,
    .knight = false,
    .neutral = false,
};

inline constexpr FactionRelations NeutralHostileRelations{
    .demon = false,
    .enemy = false,
    .wizard = false,
    .dwarf = false,
    .elf = false,
    .knight = false,
    .neutral = false,
};

[[nodiscard]] constexpr const FactionRelations& relationsFor(Faction faction)
{
    switch (faction) {
    case Faction::Demon:   return DemonHostileRelations;
    case Faction::Enemy:   return EnemyHostileRelations;
    case Faction::Wizard:  return WizardHostileRelations;
    case Faction::Dwarf:   return DwarfHostileRelations;
    case Faction::Elf:     return ElfHostileRelations;
    case Faction::Knight:  return KnightHostileRelations;
    case Faction::Neutral: return NeutralHostileRelations;
    }

    return NeutralHostileRelations;
}

[[nodiscard]] constexpr bool isHostile(Faction attacker, Faction defender)
{
    return relationsFor(attacker).relationTo(defender);
}
