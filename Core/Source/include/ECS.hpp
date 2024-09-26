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
        pool.erase(entityId);  // Remove the component if it exists
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

    ECS(){};   

    EntityID addEntity(){   
        return m_numEntities++;
    };

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
        getOrCreateComponentPool<T>().removeComponent(entityId);
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

    // void removeEntity(EntityID id);
    // template<typename T>
    void update(){
        // sort<T>();
    }

    template<typename T>
    ComponentPool<T>& view(){
        return getComponentPool<T>();
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

    template<typename T>
    std::vector<EntityID> view_sorted() {
        std::cout << "test" << std::endl;
        // Get the component pool for the given type
        ComponentPool<T>& pool = getComponentPool<T>();
        auto& componentMap = pool.getPool();

        // Create a vector of entity IDs
        std::vector<EntityID> entitiesWithComponent;
        
        // Reserve space to avoid reallocation if pool size is known
        entitiesWithComponent.reserve(pool.size());

        // Iterate through the pool and collect the entity IDs
        for (const auto& [entityID, component] : componentMap) {
            entitiesWithComponent.push_back(entityID);
        }

        // Sort the vector of entity IDs based on the 'layer' property in the component
        std::sort(entitiesWithComponent.begin(), entitiesWithComponent.end(), [&](EntityID a, EntityID b) {
            const auto& componentA = componentMap.at(a);  // Access the components only once per entity
            const auto& componentB = componentMap.at(b);
            return componentA.layer > componentB.layer;
        });

        return entitiesWithComponent;  // Return the sorted vector of entity IDs
    }
    template<typename T>
    std::vector<std::pair<EntityID, T*>> sortComponentPoolByLayer() {
        auto& pool = getComponentPool<T>();  // Assume this returns an unordered_map<EntityID, T>

        // Create a vector of pointers to components with their corresponding EntityID
        std::vector<std::pair<EntityID, T*>> sortedComponents;

        // Reserve space for efficiency
        sortedComponents.reserve(pool.size());

        // Populate the vector with entity-component pairs
        for (auto& [entityID, component] : pool) {
            sortedComponents.emplace_back(entityID, &component);
        }

        // Sort the vector based on the 'layer' property in the component
        std::sort(sortedComponents.begin(), sortedComponents.end(), [](const auto& a, const auto& b) {
            return a.second->layer < b.second->layer;  // Sorting by the layer property
        });

        return sortedComponents;  // Return sorted components
    }
private:
    // Map to store component pools for each component type
    std::unordered_map<std::type_index, std::unique_ptr<BaseComponentPool>> componentPools;
    // BasicView<Component, Component> viewCache;
    // BaseComponentPool viewCache;
    // Helper to get or create the component pool for a specific type
    template <typename T>
    ComponentPool<T>& getOrCreateComponentPool() {
        std::type_index typeIdx(typeid(T));
        if (componentPools.find(typeIdx) == componentPools.end()) {
            componentPools[typeIdx] = std::make_unique<ComponentPool<T>>();
        }
        return *reinterpret_cast<ComponentPool<T>*>(componentPools[typeIdx].get());
    }
};
