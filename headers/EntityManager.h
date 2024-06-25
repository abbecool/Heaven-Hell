#pragma once

#include <map>
#include <vector>
#include <algorithm>
#include "Entity.h"

typedef std::vector<std::shared_ptr<Entity>> EntityVec;
typedef std::map   <std::string, EntityVec> EntityMap;

class EntityManager
{
    EntityVec m_entities;
    EntityVec m_toAdd;
    EntityMap m_entityMap;
    size_t m_TotalEntities = 0;
public:
    EntityManager() {}
    void update();
    std::shared_ptr<Entity> addEntity(const std::string & tag);
    void removeDeadEntities(EntityVec & vec);
    EntityVec getEntities();
    EntityVec getEntities(std::string tag);
    size_t getTotalEntities();
};

std::shared_ptr<Entity> EntityManager::addEntity(const std::string &tag)
{   
    auto e = std::make_shared<Entity>(tag, m_TotalEntities++);
    m_toAdd.push_back(e);
    return e;
}

void EntityManager::update()
{
    for (auto e : m_toAdd)
    {
        m_entities.push_back(e);
        m_entityMap[e->tag()].push_back(e);
    }
    m_toAdd.clear();

    removeDeadEntities(m_entities);
    for ( auto& [tag, entityVec] : m_entityMap)
    {
        removeDeadEntities(entityVec);
    }
}

void EntityManager::removeDeadEntities(EntityVec & vec)
{
    for (auto e : vec)
    {
        vec.erase(std::remove_if(vec.begin(), vec.end(), [] (std::shared_ptr<Entity> e) { return !( e->isAlive() ); } ), vec.end()); 
        // vec.erase(std::ranges::begin(std::ranges::remove_if(vec, [](const auto &e){ return !e->is_alive(); })), std::end(vec));
        vec.erase(std::remove_if(vec.begin(), vec.end(), [](const auto& e) {return !e->isAlive();}), vec.end());    
    }
}

EntityVec EntityManager::getEntities()
{
    return m_entities;
}

EntityVec EntityManager::getEntities(std::string tag)
{
    return m_entityMap[tag];
}

size_t EntityManager::getTotalEntities()
{
    return m_TotalEntities;
}