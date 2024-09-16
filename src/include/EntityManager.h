#pragma once

#include "ScriptableEntity.h"
#include <iostream>

#include <map>
#include <vector>
#include <algorithm>

#include "Animation.h"
#include <memory>
#include <unordered_set>
#include <functional>

typedef Uint32 EntityID;

std::unordered_map<EntityID, CTransform> transformComponents;
std::unordered_map<EntityID, CHealth> healthComponents;

std::unordered_map<EntityID, CInputs> inputsComponents;
std::unordered_map<EntityID, CTransform> transformComponents;
std::unordered_map<EntityID, CBoundingBox> boundingBoxComponents;
std::unordered_map<EntityID, CHealth> healthComponents;
std::unordered_map<EntityID, CAnimation> animationComponents;
std::unordered_map<EntityID, CState> stateComponents;
std::unordered_map<EntityID, CProjectileState> projectileStateComponents;
std::unordered_map<EntityID, CName> nameComponents;
std::unordered_map<EntityID, CShadow> shadowComponents;
std::unordered_map<EntityID, CDamage> damageComponents;
std::unordered_map<EntityID, CDialog> dialogComponents;
std::unordered_map<EntityID, CPathfind> pathfindComponents;
std::unordered_map<EntityID, CKnockback> knockbackComponents;
std::unordered_map<EntityID, CWeapon> weaponComponents;
std::unordered_map<EntityID, CScript> scriptComponents;

class EntityManager
{
    EntityID m_TotalEntities = 0;
public:
    EntityManager();
    EntityID addEntity();

    // void removeEntity(EntityID id);
    
    template<typename Component>
    void addComponent(EntityID id, Component component);

    template<typename Component>
    bool hasComponent(EntityID id) const;

    template<typename Component>
    Component& getComponent(EntityID id);

    template<typename... Components>
    std::vector<EntityID> view();

    void update();
    void sort();
    EntityID getTotalEntities();

};

enum struct PlayerState {
    STAND = 0,
    RUN_DOWN = 1,
    RUN_RIGHT = 2,
    RUN_UP = 3,
    RUN_LEFT = 4
};

struct CComponent{
    EntityID entityId;
};

struct CInputs : CComponent
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

struct CTransform : CComponent
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

struct CBoundingBox : CComponent
{
    Vec2 size;
    Vec2 halfSize;
};

struct CHealth : CComponent
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

struct CAnimation : CComponent
{
    Animation animation;
    bool repeat = false;
};  

struct CState : CComponent
{
    PlayerState state;
    PlayerState preState; 
    bool changeAnimate = false;
}; 

struct CProjectileState : CComponent
{
        std::string state;
    bool changeAnimate = false;
}; 

struct CName : CComponent
{
    std::string name;
}; 

struct CShadow : CComponent
{
    Animation animation;
    size_t size;
};  

struct CDamage : CComponent
{
    int damage, speed, lastAttackFrame;
    std::unordered_set<std::string> damageType;
}; 

struct CDialog : CComponent
{
    Vec2 pos;
    Vec2 size;
    SDL_Texture* dialog;
};

struct CPathfind : CComponent
{
    Vec2 target;
    int target2;
};

struct CKnockback : CComponent
{
    int duration;
    int magnitude;
    int timeElapsed = 0;
    Vec2 direction;
};

struct CWeapon : CComponent
{
    Animation animation;
    int damage;
    int speed;
    int range;
    bool ranged;
    std::string type;
};

struct CScript : CComponent
{
    ScriptableEntity* Instance = nullptr;

    ScriptableEntity* (*InstantiateScript)();
    void (*DestroyInstanceScript)(CScript*);

    template<typename T>
    void Bind(){
        InstantiateScript    = []() {return static_cast<ScriptableEntity*>(new T()); }; 
        DestroyInstanceScript = [](CScript* sc) { delete sc->Instance; sc->Instance = nullptr;};
    }

};