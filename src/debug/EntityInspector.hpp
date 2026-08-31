#pragma once

#include "ecs/ECS.hpp"

#include <vector>

namespace DebugEntityInspector {

nlohmann::json inspectEntity(const ECS& ecs, EntityID entity);
std::vector<EntityID> findInspectableEntitiesAt(const ECS& ecs, Vec2 worldPoint);

} // namespace DebugEntityInspector
