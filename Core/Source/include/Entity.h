#pragma once

#include "ECS.hpp"    
#include <utility>   // For std::forward
#include <cstdint>   // For uint32_t (EntityID)

using EntityID = uint32_t;  // Define EntityID as uint32_t

class Entity {
private:
    EntityID m_entityId;
    ECS* m_ECS;

public:
    Entity() = default;  // Default constructor
    
    Entity(EntityID id, ECS* manager)
        : m_entityId(id), m_ECS(manager){};

    template<typename T>
    bool hasComponent() {
        return m_ECS->hasComponent<T>(m_entityId);  // Entity interacts with ECS
    }

    template<typename T>
    T& getComponent() {
        return m_ECS->getComponent<T>(m_entityId);  // Get component via ECS
    }

    template<typename T, typename... Args>
    T& addComponent(Args&&... args) {
        return m_ECS->addComponent<T>(m_entityId, std::forward<Args>(args)...);  // Add component via ECS
    }

    template<typename T>
    void removeComponent() {
        m_ECS->queueRemoveComponent<T>(m_entityId);  // Get component via ECS
    }

    template<typename T>
    std::vector<EntityID> view() {
        return m_ECS->signatureView<T>();
    }

    EntityID getID(){
        return m_entityId;
    }
};
