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
public:

    // Add a component to the pool for a specific entity
    template<typename... Args>
    T& addComponentOG(EntityID entityId, Args... args) {
        auto& component = pool.emplace(entityId, T(std::forward<Args>(args)...)).first->second;
        return component;
    }
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

    // Remove a component from the pool
    void removeComponentOG(EntityID entityId) {
        auto it = pool.find(entityId);
        if (it != pool.end()) {
            pool.erase(it);  // Erase element using the iterator to avoid invalidation issues
        }
    }
    void removeComponent(EntityID entityId) {
        if (entityId < poolVector.size()) {
            poolVector[entityId] = T();  // Set to default-constructed value
            poolUsed[entityId] = false;  // Mark this component as unused
        }
    }

    void queueRemoveEntityOG(EntityID entity) {
        if (entity == 1)
        {
            std::cerr << "Warning: Attempting to remove player entity." << std::endl;
            std::cout << "Type of pool: " << typeid(pool).name() << std::endl;
        }
        entitiesToRemove.push_back(entity);
    }
    void queueRemoveEntity(EntityID entity) {
        if (entity == 1)
        {
            std::cerr << "Warning: Attempting to remove player entity." << std::endl;
            std::cout << "Type of pool: " << typeid(poolVector).name() << std::endl;
        }
        entitiesToRemove.push_back(entity);
    }

    // Check if an entityId has the component
    bool hasComponentOG(EntityID entityId) const {
        if (pool.empty()) {
            return false;  // Safeguard against checking an empty pool
        }

        auto it = pool.find(entityId);
        if (it == pool.end()) {
            return false;  // Component not found for this entity
        }

        return true;  // Component exists for this entity
    }

    // Check if an entityId has the component
    const bool hasComponent(EntityID entityId) const {
        if (entityId >= poolUsed.size()) {
            return false;  // Safeguard against checking an out-of-bounds entity ID
        }
        return poolUsed[entityId];  // Component exists for this entity
    }

    T& getComponentOG(EntityID entityId)
    {
        auto it = pool.find(entityId);
        assert(it != pool.end() && "Component not found for this entity!");
        return it->second;
    }

    T& getComponent(EntityID entityId)
    {
        if (entityId >= poolVector.size() || !poolUsed[entityId]) {
            throw std::runtime_error("Component not found for this entity!");
        }
        assert(poolUsed[entityId] && "Component not found for this entity!");
        return poolVector[entityId];
    }

    // Custom iterator for range-based for loops
    class Iterator {
    public:
        using map_iterator = typename std::unordered_map<EntityID, T>::iterator;

        // Constructor
        Iterator(map_iterator it) : iter(it) {}

        // Dereference operator to return the entity ID
        EntityID operator*() const {
            return iter->first;
        }

        // Pre-increment operator
        Iterator& operator++() {
            ++iter;
            return *this;
        }

        // Equality operator
        bool operator!=(const Iterator& other) const {
            return iter != other.iter;
        }

    private:
        map_iterator iter;  // Underlying iterator for the unordered_map
    };

    // Begin iterator for range-based for loops
    Iterator begin() {
        return Iterator(pool.begin());
    }

    // End iterator for range-based for loops
    Iterator end() {
        return Iterator(pool.end());
    }

    size_t size() {
        return pool.size();
    }
    
    std::unordered_map<EntityID, T>& getPool() {
        return pool;
    }

private:
    std::unordered_map<EntityID, T> pool;  // Map of components indexed by EntityID
    std::vector<T> poolVector;  // Vector of components
};

template<typename T, typename Other> 
class BasicView : BaseComponentPool{
public:
    BasicView(){};
    // Custom iterator for range-based for loops
    class Iterator {
    public:
        using map_iterator = typename std::unordered_map<EntityID, std::tuple<T, Other>>::iterator;

        // Constructor
        Iterator(map_iterator it) : iter(it) {}

        // Dereference operator to return the entity ID
        EntityID operator*() const {
            return iter->first;
        }

        // Pre-increment operator
        Iterator& operator++() {
            ++iter;
            return *this;
        }

        // Equality operator
        bool operator!=(const Iterator& other) const {
            return iter != other.iter;
        }

    private:
        map_iterator iter;  // Underlying iterator for the unordered_map
    };

    // Begin iterator for range-based for loops
    Iterator begin() {
        return Iterator(pool.begin());
    }

    // End iterator for range-based for loops
    Iterator end() {
        return Iterator(pool.end());
    }
    void addEntity(EntityID entityID, const T& componentT, const Other& componentOther) {
        pool[entityID] = std::make_tuple(componentT, componentOther);
    }
    // Retrieve the component for an entityId
    template<typename t>
    t& getComponent(EntityID entityId) {
        auto it = pool.find(entityId);
        if (it == pool.end()) {
            throw std::runtime_error("Component not found for this entity!");
        }

        if constexpr (std::is_same_v<t, T>) {
            return std::get<0>(it->second);  // Get the first component (T) from the tuple
        } else if constexpr (std::is_same_v<t, Other>) {
            return std::get<1>(it->second);  // Get the second component (Other) from the tuple
        } else {
            throw std::runtime_error("Invalid component type requested!");
        }
    }
private:
    std::unordered_map<EntityID, std::tuple<T, Other>> pool;  // Map of components indexed by EntityID
};
