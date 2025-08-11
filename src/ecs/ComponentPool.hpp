#pragma once

#include "Components.h"

#include <iostream>
#include <unordered_map>
#include <map>
#include <vector>
#include <algorithm>
#include <memory>
#include <unordered_set>
#include <typeindex>
#include <functional>
#include <cassert>

using EntityID = uint32_t;

class BaseComponentPool {
    public:
    virtual ~BaseComponentPool() = default;  // Virtual destructor to allow proper deletion
    virtual void removeComponent(EntityID entityId){};
    std::vector<EntityID> entitiesToRemove;
    std::vector<bool> poolUsed;  // Vector to track used components
};

template<typename T>
class ComponentPool : public BaseComponentPool{
private:
    std::unordered_map<EntityID, T> pool;  // Map of components indexed by EntityID
    std::vector<T> poolVector;  // Vector of components
public:

    template<typename... Args>
    T& addComponent(EntityID entityId, Args... args) {
        if (entityId >= poolVector.size()) {
            poolVector.resize(entityId + 1);
            poolUsed.resize(entityId + 1, false);
        }
        poolVector[entityId] = T(std::forward<Args>(args)...);
        poolUsed[entityId] = true;  // Mark this component as used
        return poolVector[entityId];
    }

    void removeComponent(EntityID entityId) {
        if (entityId < poolVector.size()) {
            // poolVector[entityId] = T();  // Set to default-constructed value
            poolUsed[entityId] = false;  // Mark this component as unused
        }
    }

    void queueRemoveEntity(EntityID entity) {
        if (entity == 1)
        {
            std::cerr << "Warning: Attempting to remove player entity." << std::endl;
            std::cout << "Type of pool: " << typeid(poolVector).name() << std::endl;
        }
        entitiesToRemove.push_back(entity);
    }

    inline bool hasComponent(EntityID e) const noexcept {
        return e < poolUsed.size() && poolUsed[e];
    }

    inline T& getComponent(EntityID e) {
        if (!hasComponent(e)) {
            throw std::out_of_range("Component not found.");
        }
        return poolVector[e];
    }
};
