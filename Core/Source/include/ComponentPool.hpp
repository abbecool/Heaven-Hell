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
};

template<typename T>
class ComponentPool : public BaseComponentPool{
public:
    // Add a component to the pool for a specific entity
    
    template<typename... Args>
    T& addComponent(EntityID entityId, Args... args) {
        auto& component = pool.emplace(entityId, T(std::forward<Args>(args)...)).first->second;
        return component;
    }

    // Remove a component from the pool
    void removeComponent(EntityID entityId) {
        auto it = pool.find(entityId);
        if (it != pool.end()) {
            pool.erase(it);  // Erase element using the iterator to avoid invalidation issues
        }
    }

    void queueRemoveEntity(EntityID entity) {
        entitiesToRemove.push_back(entity);
    }

    // Check if an entityId has the component
    bool hasComponent(EntityID entityId) const {
        if (pool.empty()) {
            return false;  // Safeguard against checking an empty pool
        }

        auto it = pool.find(entityId);
        if (it == pool.end()) {
            return false;  // Component not found for this entity
        }

        return true;  // Component exists for this entity
    }

    // Retrieve the component for an entityId
    T& getComponent(EntityID entityId) {
        std::string typeName = typeid(T).name();
        
        // Check if pool is empty
        if (pool.empty()) {
            throw std::runtime_error("Pool is empty! Type: " + typeName);
        }

        // Check if the entity exists in the pool
        auto it = pool.find(entityId);
        if (it == pool.end()) {
            throw std::runtime_error("Component not found for this entity! Type: " + typeName);
        }

        // If everything is fine, return the component
        return it->second;
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

    size_t size() const {
    return pool.size();
    }
    
    std::unordered_map<EntityID, T>& getPool() {
        return pool;
    }

private:
    std::unordered_map<EntityID, T> pool;  // Map of components indexed by EntityID

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
        if constexpr (std::is_same_v<t, T>) {
            return std::get<0>(pool.at(entityId));  // Get the first component (T) from the tuple
        } else if constexpr (std::is_same_v<t, Other>) {
            return std::get<1>(pool.at(entityId));  // Get the second component (Other) from the tuple
        }
    }   
private:
    std::unordered_map<EntityID, std::tuple<T, Other>> pool;  // Map of components indexed by EntityID
};