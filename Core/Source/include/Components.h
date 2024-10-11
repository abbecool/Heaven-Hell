#pragma once

#include "Animation.h"
#include <memory>
#include <unordered_set>
#include <functional>

// set a flag: flag |= (int)PlayerState
// unset a flag: flag &= ~(int)PlayerState
// flipping a flag: flag ^= (int)PlayerState
// checking if a flag set: return (flag & (int)PlayerState) == (int)PlayerState
// checking multiple flags set: return (flag &(int)PlayerState) != 0

enum struct PlayerState {
    STAND = 0,
    RUN_DOWN = 1,
    RUN_RIGHT = 2,
    RUN_UP = 3,
    RUN_LEFT = 4
};

using EntityID = uint32_t;
class ScriptableEntity;

struct CParent
{
    EntityID parent;
    Vec2 relativePos = {0,0};
    CParent(EntityID p) : parent(p){}
    CParent(EntityID p, Vec2 relPos) : parent(p), relativePos(relPos) {}
};

struct CProjectile
{
    EntityID projectileID;
    CProjectile(EntityID p) : projectileID(p){}
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
    float speed = 0;
    bool isMovable = false;
    float tempo = 1.0f;
    CTransform() {}
    CTransform(const Vec2 & p) : pos(p), prevPos(p) {}
    CTransform(const Vec2 & p, const Vec2 & v, const Vec2 & scl, const float ang, bool mvbl) 
    : pos(p), prevPos(p), vel(v), scale(scl), angle(ang), speed(300), isMovable(mvbl){}
    CTransform(const Vec2 & p, const Vec2 & v, bool mvbl) 
        : pos(p), prevPos(p), vel(v), speed(300), isMovable(mvbl){}
    CTransform(const Vec2 & p, const Vec2 & v, const Vec2 & scl, const float ang, float spd, bool mvbl) 
    : pos(p), prevPos(p), vel(v), scale(scl), angle(ang), speed(spd), isMovable(mvbl){}
};

struct CVelocity
{
    Vec2 vel = {0, 0};    
    float angle = 0;
    float speed = 0;
    float tempo = 1.0f;
};

struct CBoundingBox
{
    Vec2 size;
    Vec2 halfSize;
    CBoundingBox() {}
    CBoundingBox(const Vec2& s) 
        : size(s), halfSize(s/2.0) {}
};

struct CImmovable
{
    CImmovable(){}    
};

struct CHealth
{
    int HP = 6;
    int HP_max = 6;
    Animation animation_full;
    Animation animation_half;
    Animation animation_empty;
    int i_frames = 30;
    size_t damage_frame = 0;
    std::unordered_set<std::string> HPType;
    CHealth() {}
    CHealth(int hp, int hp_max, int hrt_frms, const Animation& animation_full, const Animation& animation_half, const Animation& animation_empty)
        : HP(hp), HP_max(hp_max), animation_full(animation_full), animation_half(animation_half), animation_empty(animation_empty), i_frames(hrt_frms){}
};
struct CKey
{
    std::string unlocks;
    CKey() {}
    CKey(const std::string & unlcks)
        : unlocks(unlcks) {}
};

struct CAnimation
{
    Animation animation;
    bool repeat = true;
    int layer = 5;
    CAnimation() {}
    CAnimation(const Animation& animation, bool r)
                : animation(animation), repeat(r){}
    CAnimation(const Animation& animation, bool r, int l)
            : animation(animation), repeat(r), layer(l){}
};  

struct CTopLayer
{
    CTopLayer() {}
};

struct CBottomLayer
{
    CBottomLayer() {}
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
    int damage, speed;
    size_t lastAttackFrame;
    std::unordered_set<std::string> damageType;
    CDamage() {}
    CDamage(int dmg, int spd) : damage(dmg), speed(spd), lastAttackFrame(-spd) {}
    CDamage(int dmg, int spd, std::unordered_set<std::string> dmgType) : damage(dmg), speed(spd), lastAttackFrame(-spd), damageType(dmgType) {}
}; 

struct CDialog
{    
    Vec2 pos;
    Vec2 size;
    SDL_Texture * dialog;

    CDialog() {}
    CDialog(const Vec2 p, const Vec2 sz, SDL_Texture* dia) 
        : pos(p), size(sz), dialog(dia){}
};

struct CPathfind
{    
    Vec2 target;

    CPathfind() {}
    CPathfind( Vec2 trg) 
        : target(trg){}
};

struct CLoot
{
    CLoot() {}
};

struct CKnockback
{    
    int duration;
    int magnitude;
    Vec2 direction;
    int timeElapsed = 0;

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
struct CWeaponChild
{
    EntityID weaponID;
    CWeaponChild(EntityID wID)
                : weaponID(wID){}
};

struct CScript
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