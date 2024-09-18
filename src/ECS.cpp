#include "ECS.h"
#include "Entity.h"

ECS::ECS() {}

EntityID ECS::addEntity()
{   
    return m_numEntities++;
}

// Add a component to an entity
template<typename T, typename... Args>
T& addComponent(EntityID entity, Args&&... args) {
    auto& pool = getOrCreateComponentPool<T>();
    return pool.addComponent(entity, T(std::forward<Args>(args)...));
}

// Remove a component from an entityId
template <typename T>
void ECS::removeComponent(EntityID entityId) {
    getOrCreateComponentPool<T>().removeComponent(entityId);
}

 // Check if an entity has a component
template <typename T>
bool ECS::hasComponent(EntityID entityId) const {
    return getComponentPool<T>().hasComponent(entityId);
}

// Get a component from an entity
template <typename T>
T& ECS::getComponent(EntityID entityId) {
    return getComponentPool<T>().getComponent(entityId);
}

template<typename... T>
std::vector<EntityID> ECS::view(){

}

EntityID ECS::getNumEntities(){
    return m_numEntities;
}

// Helper to get or create the component pool for a specific type
template <typename T>
ComponentPool<T>& ECS::getOrCreateComponentPool() {
    std::type_index typeIdx(typeid(T));
    if (componentPools.find(typeIdx) == componentPools.end()) {
        componentPools[typeIdx] = std::make_unique<ComponentPool<T>>();
    }
    return *reinterpret_cast<ComponentPool<T>*>(componentPools[typeIdx].get());
}

// Helper to get the component pool for a specific type (const version)
template <typename T>
const ComponentPool<T>& ECS::getComponentPool() const {
    std::type_index typeIdx(typeid(T));
    return *reinterpret_cast<const ComponentPool<T>*>(componentPools.at(typeIdx).get());
}

// void ECS::update()
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

// void ECS::sort()
// {
//     std::sort(m_entities.begin(), m_entities.end(), [](const std::shared_ptr<Entity> &a, const std::shared_ptr<Entity> &b) {
//         return a->layer() > b->layer();
//     });

// }