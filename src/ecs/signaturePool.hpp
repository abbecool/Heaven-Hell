#pragma once

#include "ecs/Components.h"
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

using ComponentType = uint32_t;
const ComponentType MAX_COMPONENTS = 32;
using Signature = std::bitset<MAX_COMPONENTS>;


// Define masks for each component (bit positions) - Ordered from basic to complex
constexpr Signature CTransformMask          = 1 << 0; // 00000001, Bit 0
constexpr Signature CCollisionBoxMask       = 1 << 1; // 00000010, Bit 1
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
constexpr Signature CHitBoxMask             = 1 << 25; // Bit 25
constexpr Signature CInteractionBoxMask     = 1 << 26; // Bit 26

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

        if ((currentSignature & CChildMask) == CChildMask) {
            // std::cout << CChildMask << std::endl;
            // std::cout << currentSignature << std::endl;
            // If the entity already has said component, print a warning
            std::cerr << "Warning: Entity " << entity << " already has component of type " 
            << typeid(CChildMask).name() << ". Overwriting signature." << std::endl;
        }
        // Set the bit for the new component using bitwise OR
        currentSignature |= componentMask;

        // Update the entity's signature in the signature pool
        setSignature(entity, currentSignature);
    }

    template<typename T>
    Signature getComponentMask() {
        return componentMaskMap[typeid(T)];
    }

private:
    std::unordered_map<EntityID, Signature> m_signatures;
    std::unordered_map<std::type_index, Signature> componentMaskMap = {
        { typeid(CTransform), CTransformMask },
        { typeid(CCollisionBox), CCollisionBoxMask },
        { typeid(CHealth), CHealthMask },
        { typeid(CInputs), CInputsMask },
        { typeid(CAnimation), CAnimationMask },
        { typeid(CState), CStateMask },
        { typeid(CParent), CParentMask },
        { typeid(CShadow), CShadowMask },
        { typeid(CImmovable), CImmovableMask }, // remove when new collision system is implemented
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
        { typeid(CHitBox), CHitBoxMask },
        { typeid(CInteractionBox), CInteractionBoxMask },
    };
};
