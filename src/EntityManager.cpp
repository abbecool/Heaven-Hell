#include "EntityManager.h"


EntityManager::EntityManager() {}

std::shared_ptr<Entity> EntityManager::addEntity(const std::string &tag, const size_t &layer)
{   
    auto e = std::make_shared<Entity>(tag, m_TotalEntities++, layer);
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

void EntityManager::sort()
{
    std::sort(m_entities.begin(), m_entities.end(), [](const std::shared_ptr<Entity> &a, const std::shared_ptr<Entity> &b) {
        return a->layer() > b->layer();
    });

}

void EntityManager::removeDeadEntities(EntityVec& vec)
{
    vec.erase(
        std::remove_if(vec.begin(), vec.end(), 
        [] (const std::shared_ptr<Entity>& e) { return !e->isAlive(); }), 
        vec.end());
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