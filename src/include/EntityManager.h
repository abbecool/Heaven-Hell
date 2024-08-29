#pragma once

#include "Entity.h"
#include <iostream>

#include <map>
#include <vector>
#include <algorithm>

typedef std::vector<std::shared_ptr<Entity>> EntityVec;
typedef std::map   <std::string, EntityVec> EntityMap;
// typedef std::map   <bool, EntityVec> EntityMapBool; // Might use in the future?

class EntityManager
{
    EntityVec m_entities;
    EntityVec m_toAdd;
    EntityMap m_entityMap;
    size_t m_TotalEntities = 0;
public:
    EntityManager();
    void update();
    void sort();
    std::shared_ptr<Entity> addEntity(const std::string & tag, const size_t &layer);
    void removeDeadEntities(EntityVec & vec);
    EntityVec getEntities();
    EntityVec getEntities(std::string tag);
    EntityVec getCollisionEntities();
    size_t getTotalEntities();
};