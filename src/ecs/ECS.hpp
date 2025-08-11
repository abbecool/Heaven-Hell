#pragma once

#include "ecs/Components.h"
#include "ecs/signaturePool.hpp"
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
#include <cstdint>
#include <tuple>
#include <algorithm>
#include <limits>

using EntityID = uint32_t;

class ECS
{
private:
    friend class Entity;
    std::vector<EntityID> m_freeIDs;
    std::vector<EntityID> m_usedIDs;
    
    // Map to store component pools for each component type
    std::unordered_map<std::type_index, std::unique_ptr<BaseComponentPool>> m_componentPools;
    SignaturePool m_signaturePool;
    std::vector<EntityID> m_entitiesToRemove;

    std::vector<bool> mask;
    std::vector<EntityID> matchingEntities;

public:

    ECS(){}

    EntityID addEntity() {
        EntityID id;
        if (!m_freeIDs.empty()) {
            id = m_freeIDs.back();
            m_freeIDs.push_back(0);
        } else {
            id = m_usedIDs.size();
        }
        m_usedIDs.push_back(id);
        m_signaturePool.setSignature(id, 0);
        return id;
    }

    void removeEntity(EntityID entity){
        auto it = std::find(m_usedIDs.begin(), m_usedIDs.end(), entity);
        assert(it != m_usedIDs.end() && "Entity not found in used IDs!");
        
        if (it != m_usedIDs.end()) {
            m_usedIDs.erase(it);
        }
        else {
            std::cerr << "Warning: Attempting to remove non-existent entity " << entity << "." << std::endl;
        }
        m_freeIDs.push_back(entity);
        std::sort(m_freeIDs.begin(), m_freeIDs.end(), comp);
        
        for (auto& [type, pool]: m_componentPools) {
            if (pool != nullptr) {
                pool->removeComponent(entity);
            }
        }
        m_signaturePool.removeSignature(entity);
    }

    void queueRemoveEntity(EntityID entity, bool condition = true) {
        if (entity == 1) {
            std::cerr << "Error: Attempting to remove player entity!" << std::endl;
            
            return;
        }
        if ( hasComponent<CChild>(entity) & condition )
        {
            auto childID = getComponent<CChild>(entity).childID;
            queueRemoveComponent<CParent>(childID);
            queueRemoveComponent<CChild>(entity);
            if (childID == 1) {
                std::cerr << "Error: Attempting to remove child player entity!" << std::endl;
                return;
            }
            if ( getComponent<CChild>(entity).removeOnDeath )
            {
                m_entitiesToRemove.push_back(childID);
            }

        }
        m_entitiesToRemove.push_back(entity);
    }

    template<typename T>
    void queueRemoveComponent(EntityID entity) {
        auto& pool = getComponentPool<T>();
        pool.queueRemoveEntity(entity);

        Signature currentSignature = m_signaturePool.getSignature(entity);
        Signature componentMask = m_signaturePool.getComponentMask<T>();
        currentSignature &= ~componentMask;  // Clear the bit for the component
        m_signaturePool.setSignature(entity, currentSignature);
    }

    void update(){
        for (auto e : m_entitiesToRemove) {
            removeEntity(e);
        }
        m_entitiesToRemove.clear();

        for (auto& [type, pool] : m_componentPools) {
            if (pool == nullptr) continue;
            
            auto& entitiesToRemove = pool->entitiesToRemove;
            for (auto e : entitiesToRemove) {
                pool->removeComponent(e);
            }
            entitiesToRemove.clear();
        }
    }
    
    Signature getSignature(EntityID entity){
        return m_signaturePool.getSignature(entity);
    }
    
    // Add a component to an entity
    template<typename T, typename... Args>
    T& addComponent(EntityID entity, Args &&... args) {
        auto& pool = getOrCreateComponentPool<T>();
        m_signaturePool.addComponentToMask<T>(entity);
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
        auto it = m_componentPools.find(typeIdx);
        if (it != m_componentPools.end()) {
            return *reinterpret_cast<ComponentPool<T>*>(it->second.get());
        } else {
            static ComponentPool<T> emptyPool;
            return emptyPool;
            std::cout << "Component pool not found for type: " << typeid(T).name() << std::endl;
            // throw std::out_of_range("Component pool not found for the given type");
        }
    }
    
    template <typename T>
    ComponentPool<T>& getOrCreateComponentPool() {
        std::type_index typeIdx(typeid(T));
        if (m_componentPools.find(typeIdx) == m_componentPools.end()) {
            m_componentPools[typeIdx] = std::make_unique<ComponentPool<T>>();
        }
        return *reinterpret_cast<ComponentPool<T>*>(m_componentPools[typeIdx].get());
    }
    
    template<typename... Components>
    std::vector<EntityID> View()
    {
        // Create a vector of pointers to component pools
        std::vector<BaseComponentPool*> componentPoolsVec = { (&getOrCreateComponentPool<Components>())... };
        
        size_t minSize = componentPoolsVec[0]->poolUsed.size();
        for (auto* pool : componentPoolsVec) {
            if (pool->poolUsed.size() < minSize){
                minSize = pool->poolUsed.size();
            }
        }
        
        mask.assign(minSize, true);
        for (auto* pool : componentPoolsVec) {
            auto poolUsed = pool->poolUsed;
            for (size_t i = 0; i < mask.size(); ++i) {
                mask[i] = mask[i] && poolUsed[i];
            }
        }
        
        std::vector<EntityID> matchingEntities;
        for (EntityID entity = 0; entity < mask.size(); ++entity) {
            if (mask[entity]) {
                matchingEntities.push_back(entity);
            }
        }
        return matchingEntities;
    }
    
    void temp()
    {
        std::cout << "\rTotal entities: " << m_usedIDs.size() << ". Free IDs (" << m_freeIDs.size() << "): ";
        for (size_t i = 0; i < std::min<size_t>(m_freeIDs.size(), 40); ++i) {
            std::cout << m_freeIDs[i] << " ";
        }
        if (m_freeIDs.size() > 40)
        {
            std::cout << "...";
        }
        else{
            std::cout << "-----";
        }
        std::cout << std::flush;
    }
    
    template <typename T>
    void printComponentType() {
        std::cout << "Component type: " << typeid(T).name() << std::endl;
    }

    void status() {
        std::cout << "\r m_usedIDs size: " << m_usedIDs.size() 
                    << " | m_freeIDs size: " << m_freeIDs.size();
    }

    template<typename T>
    void component_status() {
        m_componentPools[typeid(T)]->status();
    }
};

bool comp(int a, int b) {
    return a > b;
}