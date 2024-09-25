#pragma once
#include <memory>
#include <queue>
#include <typeindex>
#include <unordered_map>

#include "Types.hpp"
#include "ComponentPool.hpp"

class EntityManager
{
public:
    EntityManager(){}

    EntityID addEntity()
    {
        return m_numEntities++;
    }

    void removeEntity(EntityID id)
    {
        m_ComponentMasks[id].reset();
    }

    void SetSignature(EntityID id, componentMask signature)
	{
		m_ComponentMasks[id] = signature;
	}

	componentMask GetSignature(EntityID id)
	{
		return m_ComponentMasks[id];
	}
private:
    EntityID m_numEntities = 0;
    std::unordered_map<EntityID, componentMask> m_ComponentMasks;
    std::unordered_map<std::type_index, std::unique_ptr<BasicComponentPool>> componentPools;
};
