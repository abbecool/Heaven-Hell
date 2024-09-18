#pragma once

// #include "ScriptableEntity.h"
#include "Animation.h"
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
    void addComponent(EntityID entityId, Args... args) {
        pool[entityId] = T(std::forward<Args>(args)...);  // Add or replace the component for the entityId
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

private:
    std::unordered_map<EntityID, T> pool;  // Map of components indexed by EntityID
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
        return pool->addComponent(entity, T(std::forward<Args>(args)...));
    };

    // Remove a component from an entityId
    template <typename T>
    void removeComponent(EntityID entityId) {
        getOrCreateComponentPool<T>().removeComponent(entityId);
    }

    // Check if an entity has a component
    template <typename T>
    bool hasComponent(EntityID entityId) const {
        return getComponentPool<T>().hasComponent(entityId);
    }

    // Get a component from an entity
    template <typename T>
    T& getComponent(EntityID entityId) {
        return getComponentPool<T>().getComponent(entityId);
    }

    template<typename... T>
    std::vector<EntityID> view(){
        return 
    };

    // void removeEntity(EntityID id);
    
    void update();
    void sort();
private:
    // Map to store component pools for each component type
    std::unordered_map<std::type_index, std::unique_ptr<BaseComponentPool>> componentPools;

    // // Helper to get or create the component pool for a specific type
    // template <typename T>
    // ComponentPool<T>& getOrCreateComponentPool();

    // // Helper to get the component pool for a specific type (const version)
    // template <typename T>
    // const ComponentPool<T>& getComponentPool() const;

    // Helper to get or create the component pool for a specific type
    template <typename T>
    ComponentPool<T>& getOrCreateComponentPool() {
        std::type_index typeIdx(typeid(T));
        if (componentPools.find(typeIdx) == componentPools.end()) {
            componentPools[typeIdx] = std::make_unique<ComponentPool<T>>();
        }
        return *reinterpret_cast<ComponentPool<T>*>(componentPools[typeIdx].get());
    }

    // Helper to get the component pool for a specific type (const version)
    template <typename T>
    const ComponentPool<T>& getComponentPool() const {
        std::type_index typeIdx(typeid(T));
        return *reinterpret_cast<const ComponentPool<T>*>(componentPools.at(typeIdx).get());
    }
};


enum struct PlayerState {
    STAND = 0,
    RUN_DOWN = 1,
    RUN_RIGHT = 2,
    RUN_UP = 3,
    RUN_LEFT = 4
};

struct CInputs
{
    bool up         = false;
    bool down       = false;
    bool left       = false;
    bool right      = false;
    bool shift      = false;
    bool ctrl       = false;
    bool shoot      = false;
    bool canShoot   = false;
};

struct CTransform
{
    Vec2 pos;    
    Vec2 prevPos;
    Vec2 vel = {0, 0};    
    Vec2 scale = {0.5, 0.5};    
    float angle = 0;
    int speed = 0;
    bool isMovable = false;
    float tempo = 1.0f;
};

struct CBoundingBox
{
    Vec2 size;
    Vec2 halfSize;
};

struct CHealth
{
    int HP;
    int HP_max;
    bool alive;
    Animation animation_full;
    Animation animation_half;
    Animation animation_empty;
    int heart_frames;
    int damage_frame = 0;
    std::unordered_set<std::string> HPType;
};

struct CAnimation
{
    Animation animation;
    bool repeat = false;
};  

struct CState
{
    PlayerState state;
    PlayerState preState; 
    bool changeAnimate = false;
}; 

struct CProjectileState
{
        std::string state;
    bool changeAnimate = false;
}; 

struct CName
{
    std::string name;
}; 

struct CShadow
{
    Animation animation;
    size_t size;
};  

struct CDamage
{
    int damage, speed, lastAttackFrame;
    std::unordered_set<std::string> damageType;
}; 

struct CDialog
{
    Vec2 pos;
    Vec2 size;
    SDL_Texture* dialog;
};

struct CPathfind
{
    Vec2 target;
    int target2;
};

struct CKnockback
{
    int duration;
    int magnitude;
    int timeElapsed = 0;
    Vec2 direction;
};

struct CWeapon
{
    Animation animation;
    int damage;
    int speed;
    int range;
    bool ranged;
    std::string type;
};

// struct CScript
// {
//     ScriptableEntity* Instance = nullptr;

//     ScriptableEntity* (*InstantiateScript)();
//     void (*DestroyInstanceScript)(CScript*);

//     template<typename T>
//     void Bind(){
//         InstantiateScript    = []() {return static_cast<ScriptableEntity*>(new T()); }; 
//         DestroyInstanceScript = [](CScript* sc) { delete sc->Instance; sc->Instance = nullptr;};
//     }
// };


// std::unordered_map<EntityID, CTransform> transformComponents;
// std::unordered_map<EntityID, CHealth> healthComponents;

// std::unordered_map<EntityID, CInputs> inputsComponents;
// std::unordered_map<EntityID, CTransform> transformComponents;
// std::unordered_map<EntityID, CBoundingBox> boundingBoxComponents;
// std::unordered_map<EntityID, CHealth> healthComponents;
// std::unordered_map<EntityID, CAnimation> animationComponents;
// std::unordered_map<EntityID, CState> stateComponents;
// std::unordered_map<EntityID, CProjectileState> projectileStateComponents;
// std::unordered_map<EntityID, CName> nameComponents;
// std::unordered_map<EntityID, CShadow> shadowComponents;
// std::unordered_map<EntityID, CDamage> damageComponents;
// std::unordered_map<EntityID, CDialog> dialogComponents;
// std::unordered_map<EntityID, CPathfind> pathfindComponents;
// std::unordered_map<EntityID, CKnockback> knockbackComponents;
// std::unordered_map<EntityID, CWeapon> weaponComponents;
// std::unordered_map<EntityID, CScript> scriptComponents;