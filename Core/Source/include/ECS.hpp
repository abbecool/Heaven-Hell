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
#include <limits>

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
constexpr Signature CSwimmingMask           = 1 << 18; // Bit 18
constexpr Signature CWaterMask              = 1 << 19; // Bit 19
constexpr Signature CScriptMask             = 1 << 20; // Bit 20
constexpr Signature CVelocityMask           = 1 << 21; // Bit 21
constexpr Signature CLifespanMask           = 1 << 22; // Bit 22
constexpr Signature CAttackMask             = 1 << 23; // Bit 23
constexpr Signature CChildMask              = 1 << 24; // Bit 24

class SignaturePool {
public:

    SignaturePool() {}
    // Add or update the signature for a given entity
    void setSignature(EntityID entity, Signature signature) {
        m_signatures[entity] = signature;
    }

    // Get the signature of a specific entity
    Signature getSignature(EntityID entity) const {
        auto it = m_signatures.find(entity);
        if (it != m_signatures.end()) {
            return it->second;
        } else {
            throw std::out_of_range("Entity signature not found");
        }
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
    // template<typename... Components, typename T>
    // std::vector<EntityID> getEntitiesWithSignature() {
    //     // Combine all component masks using bitwise OR
    //     Signature combinedMask = (getComponentMask<Components>() | ...);

    //     std::vector<EntityID> matchingEntities;

    //     // Iterate through all entities and check their signatures
    //     for (const auto& [entity, signature] : m_signatures) {
    //         // Signature signature = getSignature(entity);
    //         // Check if the entity matches the combined component mask
    //         if ((signature & combinedMask) == combinedMask) {
    //             matchingEntities.push_back(entity);
    //         }
    //     }
    //     return matchingEntities;
    // }

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
        { typeid(CSwimming), CSwimmingMask },
        { typeid(CWater), CWaterMask },
        { typeid(CWeapon), CWeaponMask },
        { typeid(CKnockback), CKnockbackMask },
        { typeid(CProjectile), CProjectileMask },
        { typeid(CProjectileState), CProjectileStateMask },
        { typeid(CLoot), CLootMask },
        { typeid(CDamage), CDamageMask },
        { typeid(CWeaponChild), CWeaponChildMask },
        { typeid(CDialog), CDialogMask },
        { typeid(CPathfind), CPathfindMask },
        { typeid(CScript), CScriptMask },
        { typeid(CVelocity), CVelocityMask },
        { typeid(CLifespan), CLifespanMask },
        { typeid(CAttack), CAttackMask },
        { typeid(CChild), CChildMask },
    };
};

class ECS
{
    EntityID m_numEntities = 0;
    friend class Entity;
public:

    ECS(){}  

    // Entity getEntity(EntityID entity){
    //     return Entity(entity, this);
    // }

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
        if ( hasComponent<CChild>(entity) )
        {
            if ( getComponent<CChild>(entity).removeOnDeath )
            {
                auto childID = getComponent<CChild>(entity).childID;
                m_entitiesToRemove.push_back(childID);
            }
        }
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

    // // View method to get entities with signature
    // template<typename... Components>
    // std::vector<EntityID> signatureView() {
    //     // Store pool sizes and pointers using a tuple
    //     auto poolData = std::make_tuple(std::make_pair(getOrCreateComponentPool<Components>().size(), 
    //                                     &getOrCreateComponentPool<Components>())...);

    //     // Find the smallest pool by size
    //     auto smallestPoolIter = std::min_element(std::begin(poolData), std::end(poolData),
    //                                             [](const auto& a, const auto& b) { return std::get<0>(a) < std::get<0>(b); });

    //     // Extract the smallest pool (component pool pointer)
    //     auto* smallestPool = std::get<1>(*smallestPoolIter);

    //     // Print the type of the smallest pool
    //     std::cout << "Type of smallest pool: " << typeid(*smallestPool).name() << std::endl;


    //     // Call getEntitiesWithSignature using the smallest pool
    //     return m_signaturePool.getEntitiesWithSignature<Components...>();
    //     // return m_signaturePool.getEntitiesWithSignature<Components...>(*reinterpret_cast<ComponentPool<std::decay_t<decltype(*smallestPool.second)>>*>(smallestPool.second));
    // }

    template <typename T>
    void printComponentType() {
        std::cout << "Smallest component type: " << typeid(T).name() << std::endl;
    }

    // template<typename... Components>
    // std::vector<EntityID> signatureView() {
    //     // Tuple of pointers to component pools
    //     auto componentPools = std::make_tuple(&getOrCreateComponentPool<Components>()...);

    //     // Variables to track the smallest pool and its size
    //     std::size_t smallestPoolSize = std::numeric_limits<std::size_t>::max();
    //     void* smallestPoolPtr = nullptr;  // Pointer to the smallest pool

    //     // Lambda to find the smallest pool
    //     std::apply([&](auto&... pools) {
    //         // Check the size of each pool
    //         (([&]{
    //             if (pools->size() < smallestPoolSize) {
    //                 smallestPoolSize = pools->size();
    //                 smallestPoolPtr = reinterpret_cast<ComponentPool<Components>*>(pools);          
    //             }
    //         }()), ...);  // Expand the lambda over the component pools
    //     }, componentPools);
        
    //     // Combine all component masks using bitwise OR
    //     Signature combinedMask = (m_signaturePool.getComponentMask<Components>() | ...);

    //     std::vector<EntityID> matchingEntities;

    //     // Iterate through all entities and check their signatures
    //     for (const auto& entity : *smallestPoolPtr) {
    //     // for (const auto& [entity, signature] : m_signatures) {
    //         Signature signature = m_signaturePool.getSignature(entity);
    //         // Check if the entity matches the combined component mask
    //         if ((signature & combinedMask) == combinedMask) {
    //             matchingEntities.push_back(entity);
    //         }
    //     }
    //     return matchingEntities;
    // }

    template<typename... Components>
    std::vector<EntityID> signatureView() {
        // Tuple of pointers to component pools
        auto componentPools = std::make_tuple(&getOrCreateComponentPool<Components>()...);

        // Variables to track the smallest pool and its size
        std::size_t smallestPoolSize = std::numeric_limits<std::size_t>::max();
        void* smallestPoolPtr = nullptr;  // Pointer to the smallest pool

        // Lambda to find the smallest pool
        std::apply([&](auto&... pools) {
            // Check the size of each pool
            (([&]{
                if (pools->size() < smallestPoolSize) {
                    smallestPoolSize = pools->size();
                    smallestPoolPtr = reinterpret_cast<void*>(pools);  // Store it as a void pointer
                }
            }()), ...);  // Expand the lambda over the component pools
        }, componentPools);
        
        // Combine all component masks using bitwise OR
        Signature combinedMask = (m_signaturePool.getComponentMask<Components>() | ...);

        std::vector<EntityID> matchingEntities;

        // Helper lambda to iterate over the smallest pool based on type
        auto iterateSmallestPool = [&](auto* pool) {
            for (const auto& entity : *pool) {
                Signature signature = m_signaturePool.getSignature(entity);
                // Check if the entity matches the combined component mask
                if ((signature & combinedMask) == combinedMask) {
                    matchingEntities.push_back(entity);
                }
            }
        };

        // Use std::apply to find the correct pool and cast the void* back to its original type
        std::apply([&](auto&... pools) {
            // Expand the lambda and compare the stored pointer to cast it back
            ((smallestPoolPtr == reinterpret_cast<void*>(pools) ? iterateSmallestPool(pools) : void()), ...);
        }, componentPools);

        return matchingEntities;
    }



private:
    // Map to store component pools for each component type
    std::unordered_map<std::type_index, std::unique_ptr<BaseComponentPool>> m_componentPools;
    SignaturePool m_signaturePool;
    std::vector<EntityID> m_entitiesToRemove;
};
