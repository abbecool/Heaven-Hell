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
constexpr Signature CKeyMask                = 1 << 13; // Bit 13
constexpr Signature CLootMask               = 1 << 14; // Bit 14
constexpr Signature CDamageMask             = 1 << 15; // Bit 15
constexpr Signature CWeaponChildMask        = 1 << 16; // Bit 16
constexpr Signature CDialogMask             = 1 << 17; // Bit 17
constexpr Signature CPathfindMask           = 1 << 18; // Bit 18
constexpr Signature CTopLayerMask           = 1 << 19; // Bit 19
constexpr Signature CBottomLayerMask        = 1 << 20; // Bit 20
constexpr Signature CScriptMask             = 1 << 21; // Bit 21


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
public:
    EntityID parent;
    Vec2 relativePos = {0,0};
    CParent(EntityID p) : parent(p){}
    CParent(EntityID p, Vec2 relPos) : parent(p), relativePos(relPos) {}
};

struct CProjectile
{
public:
    EntityID projectileID;
    CProjectile(EntityID p) : projectileID(p){}
};


struct CInputs
{
public:
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
public:
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

struct CBoundingBox
{
public:
    Vec2 size;
    Vec2 halfSize;
    CBoundingBox() {}
    CBoundingBox(const Vec2& s) 
        : size(s), halfSize(s/2.0) {}
};

struct CImmovable
{
public:
    CImmovable(){}    
};

struct CHealth
{
public:
    int HP = 6;
    int HP_max = 6;
    Animation animation_full;
    Animation animation_half;
    Animation animation_empty;
    int heart_frames = 30;
    size_t damage_frame = 0;
    std::unordered_set<std::string> HPType;
    CHealth() {}
    CHealth(int hp, int hp_max, int hrt_frms, const Animation& animation_full, const Animation& animation_half, const Animation& animation_empty)
        : HP(hp), HP_max(hp_max), animation_full(animation_full), animation_half(animation_half), animation_empty(animation_empty), heart_frames(hrt_frms){}
};
struct CKey
{
public:
    std::string unlocks;
    CKey() {}
    CKey(const std::string & unlcks)
        : unlocks(unlcks) {}
};

struct CAnimation
{
public:
    Animation animation;
    bool repeat = false;
    int layer = 5;
    CAnimation() {}
    CAnimation(const Animation& animation, bool r)
                : animation(animation), repeat(r){}
    CAnimation(const Animation& animation, bool r, int l)
            : animation(animation), repeat(r), layer(l){}
};  

struct CTopLayer
{
public:
    CTopLayer() {}
};

struct CBottomLayer
{
public:
    CBottomLayer() {}
};
struct CState
{
public:
    PlayerState state;
    PlayerState preState; 
    bool changeAnimate = false;
    CState() {}
    CState(const PlayerState s) : state(s), preState(s) {}
}; 
struct CProjectileState
{
public:
    std::string state;
    bool changeAnimate = false;
    CProjectileState() {}
    CProjectileState(std::string state ) : state(state) {}
}; 
struct CName
{
public:
    std::string name;
    CName() {}
    CName(const std::string nm) : name(nm) {}
}; 

struct CShadow
{
public:
    Animation animation;
    size_t size;
    CShadow() {}
    CShadow(const Animation& animation, size_t sz)
                : animation(animation), size(sz){}
};  

struct CDamage
{
public:
    int damage, speed, lastAttackFrame;
    std::unordered_set<std::string> damageType;
    CDamage() {}
    CDamage(int dmg, int spd) : damage(dmg), speed(spd), lastAttackFrame(-spd) {}
    CDamage(int dmg, int spd, std::unordered_set<std::string> dmgType) : damage(dmg), speed(spd), lastAttackFrame(-spd), damageType(dmgType) {}
}; 

struct CDialog
{
public:    
    Vec2 pos;
    Vec2 size;
    SDL_Texture * dialog;

    CDialog() {}
    CDialog(const Vec2 p, const Vec2 sz, SDL_Texture* dia) 
        : pos(p), size(sz), dialog(dia){}
};

struct CPathfind
{
public:    
    Vec2 target;

    CPathfind() {}
    CPathfind( Vec2 trg) 
        : target(trg){}
};

struct CLoot
{
public:
    CLoot() {}
};

struct CKnockback
{
public:    
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
public:
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
public:
    EntityID weaponID;
    CWeaponChild(EntityID wID)
                : weaponID(wID){}
};

struct CScript
{
public:
    ScriptableEntity* Instance = nullptr;

    ScriptableEntity* (*InstantiateScript)();
    void (*DestroyInstanceScript)(CScript*);

    template<typename T>
    void Bind(){
        InstantiateScript    = []() {return static_cast<ScriptableEntity*>(new T()); }; 
        DestroyInstanceScript = [](CScript* sc) { delete sc->Instance; sc->Instance = nullptr;};
    }

};