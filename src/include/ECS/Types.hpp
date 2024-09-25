#pragma once

#include <bitset>
#include <cstdint>

// ECS
using EntityID = std::uint32_t;
// const Entity MAX_ENTITIES = 10000;
using ComponentType = std::uint8_t;
const ComponentType MAX_COMPONENTS = 32;
using Signature = std::bitset<MAX_COMPONENTS>;
