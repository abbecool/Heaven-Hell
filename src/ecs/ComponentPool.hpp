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
#include <limits>

using EntityID = uint32_t;

// Base interface for deferred removal
struct BasePoolInterface {
    virtual ~BasePoolInterface() = default;
    virtual void processRemovals() = 0;
    virtual void removeComponent(EntityID entityId) = 0;
};

template<typename T>
class ComponentPool : public BasePoolInterface{
public:
    ComponentPool(size_t maxEntities = 10000) {
        // Pre-allocate sparse index map to avoid runtime resize
        sparseIndex.resize(maxEntities, npos);
        denseComponents.reserve(maxEntities);
        denseEntities.reserve(maxEntities);
    }

    // Add or update component for entity
    template<typename... Args>
    T& addComponent(EntityID entityId, Args&&... args) {
        assert(entityId < sparseIndex.size() && "EntityID exceeds maxEntities");
        size_t idx = sparseIndex[entityId];
        if (idx == npos) {
            // New component: append to dense arrays
            idx = denseComponents.size();
            sparseIndex[entityId] = idx;
            denseEntities.push_back(entityId);
            denseComponents.emplace_back(std::forward<Args>(args)...);
        } else {
            // Overwrite existing component
            denseComponents[idx] = T(std::forward<Args>(args)...);
        }
        return denseComponents[idx];
    }

    void removeComponent(EntityID entityId) {
        size_t idx = sparseIndex[entityId];
        if (idx == npos) return;

        // Erase at position idx
        denseEntities.erase(denseEntities.begin() + idx);
        denseComponents.erase(denseComponents.begin() + idx);

        // Rebuild indices for all shifted entities
        for (size_t i = idx; i <= denseEntities.size(); ++i) {
            sparseIndex[denseEntities[i]] = i;
        }
        sparseIndex[entityId] = npos;
    }

    void queueRemoveEntity(EntityID entity) {
        if (entity == 1)
        {
            std::cerr << "Warning: Attempting to remove player entity." << std::endl;
            // std::cout << "Type of pool: " << typeid(poolVector).name() << std::endl;
        }
        entitiesToRemove.push_back(entity);
    }

    // Process queued removals
    void processRemovals() {
        for (EntityID e : entitiesToRemove) {
            removeComponent(e);
        }
        entitiesToRemove.clear();
    }

    // Check if entity has component
    bool hasComponent(EntityID entityId) const noexcept {
        return entityId < sparseIndex.size() && sparseIndex[entityId] != npos;
    }

    // Get component reference (asserts if missing)
    T& getComponent(EntityID entityId) {
        assert(hasComponent(entityId) && "Component not found");
        return denseComponents[sparseIndex[entityId]];
    }

    const T& getComponent(EntityID entityId) const {
        assert(hasComponent(entityId) && "Component not found");
        return denseComponents[sparseIndex[entityId]];
    }

    // Iterate over all components (live only)
    const std::vector<T>& components() const { return denseComponents; }
    const std::vector<EntityID>& entities() const { return denseEntities; }

private:
    inline static const size_t npos = std::numeric_limits<size_t>::max();

    std::vector<size_t>        sparseIndex;      // size = maxEntities, maps EntityID -> index in dense
    std::vector<T>             denseComponents;  // actual component storage
    std::vector<EntityID>      denseEntities;    // parallel array of EntityIDs

    std::vector<EntityID>      entitiesToRemove;
};
// template<typename T>
// class ComponentPool1 : public BaseComponentPool{
// public:

//     size_t numComponents() override {
//         return poolVector.size();
//     }

//     template<typename... Args>
//     T& addComponent(EntityID entityId, Args... args) {
//         if (entityId >= poolVector.size()) {
//             poolVector.resize(entityId + 1);
//             poolUsed.resize(entityId + 1, 0);
//         }
//         poolVector[entityId] = T(std::forward<Args>(args)...);
//         poolUsed[entityId] = 1;  // Mark this component as used
//         return poolVector[entityId];
//     }

//     void removeComponent(EntityID entityId) {
//         if (entityId < poolVector.size()) {
//             // poolVector[entityId] = T();  // Set to default-constructed value
//             poolUsed[entityId] = 0;  // Mark this component as unused
//         }
//     }

//     void queueRemoveEntity(EntityID entity) {
//         if (entity == 1)
//         {
//             std::cerr << "Warning: Attempting to remove player entity." << std::endl;
//             std::cout << "Type of pool: " << typeid(poolVector).name() << std::endl;
//         }
//         entitiesToRemove.push_back(entity);
//     }

//     inline bool hasComponent(EntityID e) const noexcept {
//         return e < poolUsed.size() && poolUsed[e] == 1;
//     }

//     inline T& getComponent(EntityID e) {
//         if (!hasComponent(e)) {
//             throw std::out_of_range("Component not found.");
//         }
//         return poolVector[e];
//     }


//     void status() override {
//         size_t poolUsedSum = std::count(poolUsed.begin(), poolUsed.end(), 1);
//         std::cout << "\rComponent pool: " << typeid(T).name() 
//                   << " | poolVector size: " << poolVector.size()
//                   << " | poolUsed sum: " << poolUsedSum
//                   << " | poolUsed size: " << poolUsed.size() << "...";

//     }

// private:
//     std::unordered_map<EntityID, T> pool;  // Map of components indexed by EntityID
//     std::vector<T> poolVector;  // Vector of components
// };
