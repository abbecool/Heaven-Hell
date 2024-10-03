#pragma once

// #include "ScriptableEntity.h"
#include "Components.h"
// #include "Entity.h"
#include <iostream>
#include <unordered_map>
#include <map>
#include <vector>
#include <algorithm>
#include <memory>
#include <unordered_set>
#include <typeindex>
#include <functional>

using EntityID = uint32_t;

class BaseComponentPool {
public:
    virtual ~BaseComponentPool() = default;  // Virtual destructor to allow proper deletion
    virtual void removeComponent(EntityID entityId){};
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

    // Check if an entityId has the component
    bool hasComponent(EntityID entityId) const {
        return pool.find(entityId) != pool.end();  // Check if the entityId is in the pool
    }

    // Retrieve the component for an entityId
    T& getComponent(EntityID entityId) {
        return pool.at(entityId);  // Returns a reference to the component
    }   
    T& find(EntityID entityId) {
        return *pool.find(entityId);
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

class ECS
{
    EntityID m_numEntities = 0;
    friend class Entity;
public:

    ECS(){}  

    EntityID addEntity(){   
        return m_numEntities++;
    }

    void removeEntity(EntityID entity){
        for (auto& [type, pool]: componentPools) {
            pool->removeComponent(entity);
        }
        m_numEntities--;
    }

    EntityID getNumEntities(){
        return m_numEntities;
    }
    
    // Add a component to an entity
    template<typename T, typename... Args>
    T& addComponent(EntityID entity, Args &&... args) {
        auto& pool = getOrCreateComponentPool<T>();
        return pool.addComponent(entity, T(std::forward<Args>(args)...));
    };

    // Remove a component from an entityId
    template <typename T>
    void removeComponent(EntityID entityId) {
        getComponentPool<T>().removeComponent(entityId);
    }

    // Check if an entity has a component
    template <typename T>
    bool hasComponent(EntityID entityId) {
        return getComponentPool<T>().hasComponent(entityId);
    }

    // Get a component from an entity
    template <typename T>
    T& getComponent(EntityID entityId) {
        return getComponentPool<T>().getComponent(entityId);
    }
    // Helper to get the component pool for a specific type (const version)
    template <typename T>
    ComponentPool<T>& getComponentPool() {
        std::type_index typeIdx(typeid(T));
        return *reinterpret_cast<ComponentPool<T>*>(componentPools.at(typeIdx).get());        
    }

    template <typename T>
    ComponentPool<T>& getOrCreateComponentPool() {
        std::type_index typeIdx(typeid(T));
        if (componentPools.find(typeIdx) == componentPools.end()) {
            componentPools[typeIdx] = std::make_unique<ComponentPool<T>>();
        }
        return *reinterpret_cast<ComponentPool<T>*>(componentPools[typeIdx].get());
    }

    template<typename T>
    ComponentPool<T>& view(){
        return getOrCreateComponentPool<T>();
    };

    template<typename T, typename Other>
    BasicView<T, Other> view() {
        ComponentPool<T>& poolT = getComponentPool<T>();
        ComponentPool<Other>& poolOther = getComponentPool<Other>();
        BasicView<T, Other> viewCache;
        // viewCache = BasicView<T, Other>();

        for (const auto& [entityID, componentT] : poolT.getPool()) {
            if (poolOther.hasComponent(entityID)) {
                const Other& componentOther = poolOther.getComponent(entityID);
                viewCache.addEntity(entityID, componentT, componentOther);
            }
        }
        return viewCache;
    }
 
private:
    // Map to store component pools for each component type
    std::unordered_map<std::type_index, std::unique_ptr<BaseComponentPool>> componentPools;
};
