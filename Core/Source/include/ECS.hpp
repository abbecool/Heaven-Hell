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
#include <cstdint>
#include <tuple>
#include <algorithm>

using EntityID = uint32_t;
using ComponentType = uint32_t;
const ComponentType MAX_COMPONENTS = 32;
using Signature = std::bitset<MAX_COMPONENTS>;


// Define masks for each component (bit positions) - Ordered from basic to complex
constexpr Signature CTransformMask          = 1 << 0; // 00000001, Bit 0
constexpr Signature CBoundingBoxMask        = 1 << 1; // 00000010, Bit 1
constexpr Signature CHealthMask             = 1 << 2; // 00000100, Bit 2
constexpr Signature CInputsMask             = 1 << 3; // 00001000, Bit 3
constexpr Signature CAnimationMask          = 1 << 4; // 00010000, Bit 4
constexpr Signature CStateMask              = 1 << 5; // 00100000, Bit 5
constexpr Signature CParentMask             = 1 << 6; // 01000000, Bit 6
constexpr Signature CShadowMask             = 1 << 7; // 10000000, Bit 7
constexpr Signature CImmovableMask          = 1 << 8; // Bit 8
constexpr Signature CWeaponMask             = 1 << 9; // Bit 9
constexpr Signature CKnockbackMask          = 1 << 10; // Bit 10
constexpr Signature CProjectileMask         = 1 << 11; // Bit 11
constexpr Signature CProjectileStateMask    = 1 << 12; // Bit 12
constexpr Signature CLootMask               = 1 << 13; // Bit 13
constexpr Signature CDamageMask             = 1 << 14; // Bit 14
constexpr Signature CWeaponChildMask        = 1 << 15; // Bit 15
constexpr Signature CDialogMask             = 1 << 16; // Bit 16
constexpr Signature CPathfindMask           = 1 << 17; // Bit 17
constexpr Signature CTopLayerMask           = 1 << 18; // Bit 18
constexpr Signature CBottomLayerMask        = 1 << 19; // Bit 19
constexpr Signature CScriptMask             = 1 << 20; // Bit 20
constexpr Signature CVelocityMask           = 1 << 21; // Bit 21

class SignaturePool {
public:

    SignaturePool() {}
    // Add or update the signature for a given entity
    void setSignature(EntityID entity, Signature signature) {
        m_signatures[entity] = signature;
    }

    // Get the signature of a specific entity
    Signature getSignature(EntityID entity) const {
        return m_signatures.at(entity);
    }

    // Remove the signature for a given entity
    void removeSignature(EntityID entity) {
        m_signatures.erase(entity);
    }

    template<typename T>
    void addComponentToMask(EntityID entity) {
        Signature componentMask = getComponentMask<T>();
        // Retrieve the current signature of the entity
        Signature currentSignature = getSignature(entity);

        // Set the bit for the new component using bitwise OR
        currentSignature |= componentMask;

        // Update the entity's signature in the signature pool
        setSignature(entity, currentSignature);
    }

    template<typename T>
    Signature getComponentMask() {
        return componentMaskMap[typeid(T)];
    }

    // Function to get all entities matching a given component signature for any number of components
    template<typename... Components>
    std::vector<EntityID> getEntitiesWithSignature() {
        // Combine all component masks using bitwise OR
        Signature combinedMask = (getComponentMask<Components>() | ...);

        std::vector<EntityID> matchingEntities;

        // Iterate through all entities and check their signatures
        for (const auto& [entity, signature] : m_signatures) {
            // Signature entitySignature = getSignature(entity);
            // Check if the entity matches the combined component mask
            if ((signature & combinedMask) == combinedMask) {
                matchingEntities.push_back(entity);
            }
        }
        return matchingEntities;
    }

private:
    std::unordered_map<EntityID, Signature> m_signatures;
    std::unordered_map<std::type_index, Signature> componentMaskMap = {
        { typeid(CTransform), CTransformMask },
        { typeid(CBoundingBox), CBoundingBoxMask },
        { typeid(CHealth), CHealthMask },
        { typeid(CInputs), CInputsMask },
        { typeid(CAnimation), CAnimationMask },
        { typeid(CState), CStateMask },
        { typeid(CParent), CParentMask },
        { typeid(CShadow), CShadowMask },
        { typeid(CImmovable), CImmovableMask },
        { typeid(CWeapon), CWeaponMask },
        { typeid(CKnockback), CKnockbackMask },
        { typeid(CProjectile), CProjectileMask },
        { typeid(CProjectileState), CProjectileStateMask },
        { typeid(CLoot), CLootMask },
        { typeid(CDamage), CDamageMask },
        { typeid(CWeaponChild), CWeaponChildMask },
        { typeid(CDialog), CDialogMask },
        { typeid(CPathfind), CPathfindMask },
        { typeid(CTopLayer), CTopLayerMask },
        { typeid(CBottomLayer), CBottomLayerMask },
        { typeid(CScript), CScriptMask },
        { typeid(CVelocity), CVelocityMask },
    };
};

class ECS
{
    EntityID m_numEntities = 0;
    friend class Entity;
public:

    ECS(){}  

    EntityID addEntity(){   
        m_numEntities++;
        m_signaturePool.setSignature(m_numEntities, 0);
        return m_numEntities;
    }

    void removeEntity(EntityID entity){
        for (auto& [type, pool]: m_componentPools) {
            if (pool != nullptr) {
                pool->removeComponent(entity);
            }
        }
        m_signaturePool.removeSignature(entity);
    }

    void queueRemoveEntity(EntityID entity) {
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
        return *reinterpret_cast<ComponentPool<T>*>(m_componentPools.at(typeIdx).get());        
    }

    template <typename T>
    ComponentPool<T>& getOrCreateComponentPool() {
        std::type_index typeIdx(typeid(T));
        if (m_componentPools.find(typeIdx) == m_componentPools.end()) {
            m_componentPools[typeIdx] = std::make_unique<ComponentPool<T>>();
        }
        return *reinterpret_cast<ComponentPool<T>*>(m_componentPools[typeIdx].get());
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

    // View method to get entities with signature
    template<typename... Components>
    std::vector<EntityID> signatureView() {
        // Store pool sizes and pointers
        std::vector<std::pair<std::size_t, BaseComponentPool*>> poolData;

        // Get the sizes and store them in vector
        (poolData.emplace_back(getOrCreateComponentPool<Components>().size(), &getOrCreateComponentPool<Components>()), ...);

        // Find the smallest pool
        auto smallestPool = *std::min_element(poolData.begin(), poolData.end(),
                                              [](const auto& a, const auto& b) { return a.first < b.first; });

        std::cout << smallestPool << std::endl;

        // Call getEntitiesWithSignature using the smallest pool
        return m_signaturePool.getEntitiesWithSignature<Components...>();
        // return m_signaturePool.getEntitiesWithSignature<Components...>(*reinterpret_cast<ComponentPool<std::decay_t<decltype(*smallestPool.second)>>*>(smallestPool.second));
    }


private:
    // Map to store component pools for each component type
    std::unordered_map<std::type_index, std::unique_ptr<BaseComponentPool>> m_componentPools;
    SignaturePool m_signaturePool;
    std::vector<EntityID> m_entitiesToRemove;
};
