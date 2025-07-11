#pragma once

#include "Animation.h"
#include <memory>
#include <unordered_set>
#include <functional>
#include <bitset>

constexpr uint8_t MAX_LAYERS = 8;

// using ChildMask = std::bitset<MAX_LAYERS>;
// constexpr ChildMask EMPTY_MASK              = 0;        // 00000000, No bits set
// constexpr ChildMask SHADOW_LAYER            = 1 << 0;   // 00000001, Bit 1
// constexpr ChildMask PRIMARY_WEAPON_LAYER    = 1 << 1;   // 00000010, Bit 2
// constexpr ChildMask SECONDARY_WEAPON_LAYER  = 1 << 2;   // 00000100, Bit 3
// constexpr ChildMask DIALOG_LAYER            = 1 << 3;   // 00001000, Bit 4
// constexpr ChildMask HEALTH_BAR_LAYER        = 1 << 4;   // 00010000, Bit 5

// constexpr ChildMask DAMAGE_LAYER            = 1 << 5;   // 00100000, Bit 6
// constexpr ChildMask WATER_LAYER             = 1 << 6;   // 01000000, Bit 7
// constexpr ChildMask FINAL_MASK              = 1 << 7;   // 10000000, Final bit set

using CollisionMask = std::bitset<MAX_LAYERS>;
constexpr CollisionMask EMPTY_MASK              = 0;        // 00000000, No bits set
constexpr CollisionMask PLAYER_LAYER            = 1 << 0;   // 00000001, Bit 1
constexpr CollisionMask ENEMY_LAYER             = 1 << 1;   // 00000010, Bit 2
constexpr CollisionMask PROJECTILE_LAYER        = 1 << 2;   // 00000100, Bit 3
constexpr CollisionMask OBSTACLE_LAYER          = 1 << 3;   // 00001000, Bit 4
constexpr CollisionMask FRIENDLY_LAYER          = 1 << 4;   // 00010000, Bit 5
constexpr CollisionMask DAMAGE_LAYER            = 1 << 5;   // 00100000, Bit 6
constexpr CollisionMask WATER_LAYER             = 1 << 6;   // 01000000, Bit 7
constexpr CollisionMask FINAL_MASK              = 1 << 7;   // 10000000, Final bit set

using InterationMask = std::bitset<MAX_LAYERS>;
// constexpr InterationMask EMPTY_MASK                 = 0;         // 00000000, No bits set
constexpr InterationMask PLAYER_LAYER1              = 1 << 0;   // 00000001, Bit 0
constexpr InterationMask LOOT_LAYER                 = 1 << 1;   // 00000010, Bit 1
// constexpr InterationMask ENEMY_LAYER                = 1 << 2;    // 00000100, Bit 2
constexpr InterationMask FRIENDLY_LAYER1            = 1 << 3;   // 00001000, Bit 3
// constexpr InterationMask DAMAGE_LAYER               = 1 << 4;    // 00010000, Bit 4
// constexpr InterationMask INTERACTABLE_LAYER         = 1 << 5;    // 00100000, Bit 5
// constexpr InterationMask PROJECTILE_LAYER           = 1 << 6;    // 01000000, Bit 6
// constexpr InterationMask FINAL_MASK                 = 1 << 7;    // 10000000, Final bit set

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
    bool interact   = false;
    bool shoot      = false;
    bool canShoot   = false;
    CInputs() {};
};

struct CTransform
{
    Vec2 pos;    
    Vec2 prevPos;
    Vec2 vel = {0, 0};    
    Vec2 scale = {1, 1};    
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

struct CBox 
{
    Vec2 size;
    Vec2 halfSize;
    CollisionMask layer = FINAL_MASK;
    CollisionMask mask = EMPTY_MASK; // bitmask of layers this entity should collide with
    Uint8 red;
    Uint8 green;
    Uint8 blue;

    CBox() {}
    CBox(const Vec2& s) 
        : size(s), halfSize(s/2.0), red(255), green(255), blue(255) {}
    CBox(const Vec2& s, CollisionMask l, CollisionMask m, const Uint8 r, const Uint8 g, const Uint8 b) // only use this after the new collision system is implemented
        : size(s), halfSize(s/2.0), layer(l), mask(m), red(r), green(g), blue(b) {}
};

struct CCollisionBox : public CBox 
{
    CCollisionBox() {}
    CCollisionBox(const Vec2& s) 
        : CBox(s) {}
    CCollisionBox(const Vec2& s, const Uint8 r, const Uint8 g, const Uint8 b) 
        : CBox(s) {}

    CCollisionBox(const Vec2& size, CollisionMask layer, CollisionMask mask) // only use this after the new collision system is implemented
        : CBox(size, layer, mask, 255, 255, 255) {}
};

struct CHitBox
{
    Vec2 size;
    Vec2 halfSize;

    CHitBox() {}
};

struct CInteractionBox : public CBox
{
    CInteractionBox(const Vec2& size, CollisionMask layer, CollisionMask mask) // only use this after the new collision system is implemented
        : CBox(size, layer, mask, 0, 0, 255) {}
};

struct CImmovable
{
    CImmovable(){}    
};

struct CWater {
    bool isDeep = false;  // Differentiates deep vs shallow water
};

struct CSwimming
{
    bool isSwimming = false;
    float swimSpeedMultiplier = 0.5f;
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

struct CLifespan
{
    int lifespan;
    CLifespan(int lf) : lifespan(lf){}
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

struct CAttack
{
    int damage = 1;
    int speed = 120;
    int attackTimer = 120;
    int duration = 30;
    int range = 3*64;
    Vec2 area = {16, 16};
    CAttack() {}
    CAttack(int dmg, int spd, int dur, int rg, Vec2 ae) : damage(dmg), speed(spd), attackTimer(spd), duration(dur), range(rg), area(ae){}
};

struct CDamage
{
    int damage;
    std::unordered_set<std::string> damageType;
    CDamage() {}
    CDamage(int dmg) : damage(dmg) {}
    CDamage(int dmg, std::unordered_set<std::string> dmgType) : damage(dmg), damageType(dmgType) {}
}; 

struct CDialog
{    
    Vec2 size;
    SDL_Texture * dialog;
    std::string dialog_text;
    Vec2 offset = Vec2{0, 1};

    CDialog() {}
    CDialog(const Vec2 sz, SDL_Texture* dia, std::string txt) 
        : size(sz), dialog(dia), dialog_text(txt){}
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

struct CChild
{

    std::vector<std::tuple<EntityID, bool>> children;

    EntityID childID;
    bool removeOnDeath;
    CChild(EntityID cID)
                : childID(cID), removeOnDeath(true){}
    CChild(EntityID cID, bool remove)
                : childID(cID), removeOnDeath(remove){}
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

struct CChunk
{
    Vec2 chunkPos;
    std::vector<EntityID> chunkChildern;
    CChunk(Vec2 cPos)
            : chunkPos(cPos){}
};
