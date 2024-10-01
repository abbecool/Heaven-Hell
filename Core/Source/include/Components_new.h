#pragma once

#include "Animation.h"
#include <memory>
#include <unordered_set>
#include <functional>

enum struct PlayerState {
    STAND = 0,
    RUN_DOWN = 1,
    RUN_RIGHT = 2,
    RUN_UP = 3,
    RUN_LEFT = 4
};

struct CComponent
{

};

struct CInputs : public CComponent
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

struct CTransform : public CComponent
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

struct CBoundingBox : public CComponent
{
    Vec2 size;
    Vec2 halfSize;
};

struct CHealth : public CComponent
{
    int HP;
    int HP_max;
    Animation animation_full;
    Animation animation_half;
    Animation animation_empty;
    int heart_frames;
    int damage_frame = 0;
    std::unordered_set<std::string> HPType;
};

struct CAnimation : public CComponent
{
    Animation animation;
    bool repeat = false;
    int layer;
};  

struct CState : public CComponent
{
    PlayerState state;
    PlayerState preState; 
    bool changeAnimate = false;
}; 

struct CProjectileState : public CComponent
{
        std::string state;
    bool changeAnimate = false;
}; 

struct CName : public CComponent
{
    std::string name;
}; 

struct CShadow : public CComponent
{
    Animation animation;
    size_t size;
};  

struct CDamage : public CComponent
{
    int damage, speed, lastAttackFrame;
    std::unordered_set<std::string> damageType;
}; 

struct CDialog : public CComponent
{
    Vec2 pos;
    Vec2 size;
    SDL_Texture* dialog;
};

struct CPathfind : public CComponent
{
    Vec2 target;
    int target2;
};

struct CKnockback : public CComponent
{
    int duration;
    int magnitude;
    int timeElapsed = 0;
    Vec2 direction;
};

struct CWeapon : public CComponent
{
    Animation animation;
    int damage;
    int speed;
    int range;
    bool ranged;
    std::string type;
};

// struct CScript : public CComponent
// {
// //     ScriptableEntity* Instance = nullptr;

//     ScriptableEntity* (*InstantiateScript)();
//     void (*DestroyInstanceScript)(CScript*);

//     template<typename T>
//     void Bind(){
//         InstantiateScript    = []() {return static_cast<ScriptableEntity*>(new T()); }; 
//         DestroyInstanceScript = [](CScript* sc) { delete sc->Instance; sc->Instance = nullptr;};
//     }

// };