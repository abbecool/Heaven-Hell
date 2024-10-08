#pragma once

#include "Components.h"
#include "ComponentPool.hpp"

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
#include <bitset>

using EntityID = uint32_t;
using ComponentType = uint8_t;
const ComponentType MAX_COMPONENTS = 32;
using Signature = std::bitset<MAX_COMPONENTS>;


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
            if (pool != nullptr) {
                pool->removeComponent(entity);
            }
        }
        // m_numEntities--;
    }

    void queueRemoveEntity(EntityID entity) {
        m_entitiesToRemove.push_back(entity);
    }

    template<typename T>
    void queueRemoveComponent(EntityID entity) {
        auto& pool = getOrCreateComponentPool<T>();
        pool.queueRemoveEntity(entity);
    }

    void update(){
        for (auto e : m_entitiesToRemove) {
            removeEntity(e);
        }
        m_entitiesToRemove.clear();

        for (auto& [type, pool] : componentPools) {
            if (pool != nullptr) {
                auto& entitiesToRemove = pool->entitiesToRemove;
                for (auto e : entitiesToRemove) {
                    pool->removeComponent(e);
                }
                entitiesToRemove.clear();
            }
        }
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
        return getOrCreateComponentPool<T>().hasComponent(entityId);
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
    std::unordered_map<EntityID, Signature> signaturePool;
    std::vector<EntityID> m_entitiesToRemove;
};
