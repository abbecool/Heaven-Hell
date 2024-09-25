#pragma once

#include "ECS/EntityManager.hpp"    
#include <utility>   // For std::forward
#include <cstdint>   // For uint32_t (EntityID)

using EntityID = uint32_t;  // Define EntityID as uint32_t

class Entity_new {
private:
    EntityID m_entityId;
    EntityManager* m_EntityManager;

public:
    Entity_new() = default;  // Default constructor
    
    Entity_new(EntityID id, EntityManager* manager)
        : m_entityId(id), m_EntityManager(manager){};

    // template<typename T>
    // bool hasComponent() {
    //     return m_EntityManager->hasComponent<T>(m_entityId);  // Entity interacts with ECS
    // }

    // template<typename T>
    // T& getComponent() {
    //     return m_EntityManager->getComponent<T>(m_entityId);  // Get component via ECS
    // }

    // template<typename T, typename... Args>
    // T& addComponent(Args&&... args) {
    //     return m_EntityManager->addComponent<T>(m_entityId, std::forward<Args>(args)...);  // Add component via ECS
    // }

    EntityID getID(){
        return m_entityId;
    }
};