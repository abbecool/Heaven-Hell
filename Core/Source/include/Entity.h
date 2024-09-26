#pragma once

#include "ECS.hpp"    
#include <utility>   // For std::forward
#include <cstdint>   // For uint32_t (EntityID)

using EntityID = uint32_t;  // Define EntityID as uint32_t

class Entity {
private:
    EntityID m_entityId;
    ECS* m_ECS;

public:
    Entity() = default;  // Default constructor
    
    Entity(EntityID id, ECS* manager)
        : m_entityId(id), m_ECS(manager){};

    template<typename T>
    bool hasComponent() {
        return m_ECS->hasComponent<T>(m_entityId);  // Entity interacts with ECS
    }

    template<typename T>
    T& getComponent() {
        return m_ECS->getComponent<T>(m_entityId);  // Get component via ECS
    }

    template<typename T, typename... Args>
    T& addComponent(Args&&... args) {
        return m_ECS->addComponent<T>(m_entityId, std::forward<Args>(args)...);  // Add component via ECS
    }

    EntityID getID(){
        return m_entityId;
    }
};


// #include <memory>
// #include "Components.h"
// #include <tuple>
// #include <string>
// #include <unordered_map>
// #include <unordered_set>

// typedef std::tuple<
//     CTransform,
//     CInputs,
//     CBoundingBox,
//     CAnimation,
//     CKey,
//     CState,
//     CProjectileState,
//     CHealth,    
//     CName,  
//     CShadow,    
//     CDamage,
//     CDialog,
//     CPathfind,
//     CKnockback,
//     CWeapon,
//     CScript
// > ComponentTuple;

// class Entity;

// struct Link {
//     std::shared_ptr<Entity> linkEntity;  // Reference to the weapon entity

//     Link(){}
//     Link(std::shared_ptr<Entity> lE) : linkEntity(lE) {}
// };

// class Entity
// {
//     friend class ECS;

//     const std::string   m_tag   = "Default";
//     const size_t        m_id    = 0;
//     const size_t        m_layer    = 0;
//     bool                m_alive = true;
//     bool                m_inCamera = true;
//     ComponentTuple      m_components;
//     Link                m_link;
//     std::unordered_map<std::string, std::unordered_set<std::string>> m_effectiveDamageToEnemyMap = {
//         {"Fire",        {"Ice", "Grass", }},
//         {"Water",       {"Fire", "Rock"}},
//         {"Lightning",   {"Water", "Air"}},
//         {"Ice",         {"Grass", "Air"}},
//         {"Rock",        {"Ice", "Lightning"}},
//         {"Air",         {"Air"}},

//         {"Piercing",    {"Armored", "Flesh"}},
//         {"Slashing",    {"Flesh", "Organic"}},
//         {"Blunt",       {"Armored"}},
//         {"Explosive",   {"Armored"}},

//         {"Light",       {"Dark"}},
//         {"Dark",        {"Light"}}
//     };
//     std::unordered_map<std::string, std::unordered_set<std::string>> m_ineffectiveDamageToEnemyMap = {
//         {"Fire",        {"Water", "Rock", }},
//         {"Water",       {"Ice", "Grass"}},
//         {"Lightning",   {"Rock", "Fire"}},
//         {"Ice",         {"Grass", "Air"}},
//         {"Rock",        {"Water", "Rock"}},
//         {"Air",         {"Rock", "Lightning"}},

//         {"Piercing",    {}},
//         {"Slashing",    {"Armored"}},
//         {"Blunt",       {}},
//         {"Explosive",   {}},

//         {"Light",       {"Light"}},
//         {"Dark",        {"Dark"}}
//     };


// public:

//     Entity(const std::string& tag, const size_t id, const size_t layer);
//     bool isAlive() const;
//     bool inCamera() const;
//     void setInCamera(bool set);
//     const std::string& tag() const;
//     const size_t layer() const;
//     const size_t id() const;
//     const bool movable() const;
//     void kill();
//     void setLinkEntity(std::shared_ptr<Entity>);
//     std::shared_ptr<Entity> getLinkEntity();
//     void removeLinkEntity();

//     bool isTag(std::string tag) const;

//     template<typename T>
//     bool hasComponent() const {
//         return getComponent<T>().has;
//     }

//     template<typename T, typename... TArgs>
//     T& addComponent(TArgs&&... mArgs) {
//         auto& component = getComponent<T>();
//         component = T(std::forward<TArgs>(mArgs)...);
//         component.has = true;
//         return component;
//     }

//     template<typename T>
//     T& getComponent() {
//         return std::get<T>(m_components);
//     }

//     template<typename T>
//     const T& getComponent() const {
//         return std::get<T>(m_components);
//     }

//     template<typename T>
//     void removeComponent() {
//         getComponent<T>() = T();
//     }

//     void movePosition(Vec2);
//     void setScale(Vec2 scale);
//     bool takeDamage(std::shared_ptr<Entity> attacker, size_t frame);
// };
