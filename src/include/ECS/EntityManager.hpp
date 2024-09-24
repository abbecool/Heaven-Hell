#pragma once
#include <memory>
#include <queue>

using EntityID = uint32_t;

class EntityManager
{
private:
    std::queue<EntityID> m_AvailableEntities;
public:
    EntityManager(){}

    EntityID addEntity(){}

    void removeEntity(){}
};
