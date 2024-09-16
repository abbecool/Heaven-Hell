#include "EntityManager.h"


EntityManager::EntityManager() {}

EntityID EntityManager::addEntity()
{   
    m_TotalEntities++;
    return m_TotalEntities;
}

template<typename Component>
void EntityManager::addComponent(EntityID id, Component component){
    transformComponents[id]
}

template<typename Component>
bool EntityManager::hasComponent(EntityID id) const{
    return transformComponents[id];
}

template<typename Component>
Component& EntityManager::getComponent(EntityID id){

}

template<typename... Components>
std::vector<EntityID> EntityManager::view(){

}

// void EntityManager::update()
// {
//     for (auto e : m_toAdd)
//     {
//             m_entities.push_back(e);
//             m_entityMap[e->tag()].push_back(e);
//     }
//     m_toAdd.clear();

//     removeDeadEntities(m_entities);
//     for ( auto& [tag, entityVec] : m_entityMap)
//     {
//         removeDeadEntities(entityVec);
//     }
// }

// void EntityManager::sort()
// {
//     std::sort(m_entities.begin(), m_entities.end(), [](const std::shared_ptr<Entity> &a, const std::shared_ptr<Entity> &b) {
//         return a->layer() > b->layer();
//     });

// }

EntityID EntityManager::getTotalEntities(){
    return m_TotalEntities;
}

