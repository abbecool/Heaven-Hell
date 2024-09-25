#pragma once
#include <memory>
#include <queue>

#include "Types.hpp"

using EntityID = uint32_t;

class EntityManager
{
private:
    EntityID m_numEntities = 0;
public:
    EntityManager(){}

    EntityID addEntity()
    {
        return m_numEntities++;
    }

    void removeEntity(){}
};
