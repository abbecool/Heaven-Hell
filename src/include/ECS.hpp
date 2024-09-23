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

    std::unordered_map<EntityID, T> pool;  // Map of components indexed by EntityID
private:
};

template<typename T, typename Other> 
class BasicView : BaseComponentPool{
public:
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
    std::unordered_map<EntityID, std::tuple<T, Other>> pool;  // Map of components indexed by EntityID
private:
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
        BasicView<T, Other> view;

        for (const auto& [entityID, componentT] : poolT.getPool()) {
            if (poolOther.hasComponent(entityID)) {
                const Other& componentOther = poolOther.getComponent(entityID);
                view.addEntity(entityID, componentT, componentOther);
            }
        }

        return view;
    }

    template<typename T>
    std::vector<EntityID> view_sorted() {
        // Get the component pool for the given type
        ComponentPool<T>& pool = getComponentPool<T>();

        // Create a vector of entity IDs
        std::vector<EntityID> entitiesWithComponent;
        
        // Reserve space to avoid reallocation if pool size is known
        entitiesWithComponent.reserve(pool.size());

        // Iterate through the pool and collect the entity IDs
        for (const auto& [entityID, component] : pool.getPool()) {
            entitiesWithComponent.push_back(entityID);
        }

        // Sort the vector of entity IDs based on the 'layer' property in the component
        std::sort(entitiesWithComponent.begin(), entitiesWithComponent.end(), [&](EntityID a, EntityID b) {
            return pool.getPool().at(a).layer > pool.getPool().at(b).layer;  // Compare layer values of components
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
    CInputs() {};
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
    CTransform() {}
    CTransform(const Vec2 & p) : pos(p), prevPos(p) {}
    CTransform(const Vec2 & p, const Vec2 & v, const Vec2 & scl, const float ang, bool mvbl) 
    : pos(p), prevPos(p), vel(v), scale(scl), angle(ang), speed(300), isMovable(mvbl){}
    CTransform(const Vec2 & p, const Vec2 & v, bool mvbl) 
        : pos(p), prevPos(p), vel(v), speed(300), isMovable(mvbl){}
    CTransform(const Vec2 & p, const Vec2 & v, const Vec2 & scl, const float ang, int spd, bool mvbl) 
    : pos(p), prevPos(p), vel(v), scale(scl), angle(ang), speed(spd), isMovable(mvbl){}
};

struct CBoundingBox
{
    Vec2 size;
    Vec2 halfSize;
    CBoundingBox() {}
    CBoundingBox(const Vec2& s) 
        : size(s), halfSize(s/2.0) {}
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
    CHealth() {}
    CHealth(int hp, int hp_max, int hrt_frms, const Animation& animation_full, const Animation& animation_half, const Animation& animation_empty)
        : HP(hp), HP_max(hp_max), animation_full(animation_full), animation_half(animation_half), animation_empty(animation_empty), heart_frames(hrt_frms){}
};

struct CAnimation
{
    Animation animation;
    bool repeat = false;
    int layer = 0;
    CAnimation() {}
    CAnimation(const Animation& animation, bool r)
                : animation(animation), repeat(r){}
    CAnimation(const Animation& animation, bool r, int l)
                : animation(animation), repeat(r), layer(l){}
};  

struct CState
{
    PlayerState state;
    PlayerState preState; 
    bool changeAnimate = false;
    CState() {}
    CState(const PlayerState s) : state(s), preState(s) {}
}; 

struct CProjectileState
{
    std::string state;
    bool changeAnimate = false;
    CProjectileState() {}
    CProjectileState(std::string state ) : state(state) {}
}; 

struct CName
{
    std::string name;
    CName() {}
    CName(const std::string nm) : name(nm) {}
}; 

struct CShadow
{
    Animation animation;
    size_t size;
    CShadow() {}
    CShadow(const Animation& animation, size_t sz)
                : animation(animation), size(sz){}
};  

struct CDamage
{
    int damage, speed, lastAttackFrame;
    std::unordered_set<std::string> damageType;
    CDamage() {}
    CDamage(int dmg, int spd) : damage(dmg), speed(spd), lastAttackFrame(-spd) {}
    CDamage(int dmg, int spd, std::unordered_set<std::string> dmgType) : damage(dmg), speed(spd), lastAttackFrame(-spd), damageType(dmgType) {}
}; 

struct CDialog
{
    Vec2 pos;
    Vec2 size;
    SDL_Texture* dialog;
    CDialog() {}
    CDialog(const Vec2 p, const Vec2 sz, SDL_Texture* dia) 
        : pos(p), size(sz), dialog(dia){}
};

struct CPathfind
{
    Vec2 target;
    EntityID target2;
    CPathfind() {}
    CPathfind( Vec2 trg, EntityID trg2 ) 
        : target(trg), target2(trg2) {}
};

struct CKnockback
{
    int duration;
    int magnitude;
    int timeElapsed = 0;
    Vec2 direction;
    CKnockback() {}
    CKnockback( int dur, int mag, Vec2 dir) 
        : duration(dur), magnitude(mag), direction(dir) {}
};

struct CWeapon
{
    Animation animation;
    int damage;
    int speed;
    int range;
    bool ranged;
    std::string type;
    CWeapon() {}
    CWeapon(const Animation& animation, int damage, int speed, int range)
                : animation(animation), damage(damage), speed(speed), range(range){}
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