#pragma once

#include "assets/SpriteDefinition.hpp"
#include "physics/InventoryManager.hpp"
#include "render/RenderTypes.hpp"
#include "story/EventBus.hpp"

#include <memory>
#include <unordered_set>
#include <functional>
#include <bitset>
#include <algorithm>

using json = nlohmann::json;

constexpr uint8_t MAX_LAYERS = 16;

using CollisionMask = std::bitset<MAX_LAYERS>;
constexpr CollisionMask EMPTY_MASK              = 0;        // 00000000, No bits set
constexpr CollisionMask PLAYER_LAYER            = 1 << 0;   // 00000001, Bit 1
constexpr CollisionMask ENEMY_LAYER             = 1 << 1;   // 00000010, Bit 2
constexpr CollisionMask PROJECTILE_LAYER        = 1 << 2;   // 00000100, Bit 3
constexpr CollisionMask OBSTACLE_LAYER          = 1 << 3;   // 00001000, Bit 4
constexpr CollisionMask FRIENDLY_LAYER          = 1 << 4;   // 00010000, Bit 5
constexpr CollisionMask DAMAGE_LAYER            = 1 << 5;   // 00100000, Bit 6
constexpr CollisionMask WATER_LAYER             = 1 << 6;   // 01000000, Bit 7
constexpr CollisionMask LOOT_LAYER              = 1 << 7;   // 10000000, Bit 8
constexpr CollisionMask AREA_LAYER              = 1 << 8;   // 10000000, Final bit set

inline std::unordered_map<std::string, CollisionMask> componentMaskMap = 
{
    {"EMPTY_MASK", EMPTY_MASK},
    {"PLAYER_LAYER", PLAYER_LAYER},
    {"ENEMY_LAYER", ENEMY_LAYER},
    {"PROJECTILE_LAYER", PROJECTILE_LAYER},
    {"OBSTACLE_LAYER", OBSTACLE_LAYER},
    {"FRIENDLY_LAYER", FRIENDLY_LAYER},
    {"DAMAGE_LAYER", DAMAGE_LAYER},
    {"WATER_LAYER", WATER_LAYER},
    {"LOOT_LAYER", LOOT_LAYER},
    {"AREA_LAYER", AREA_LAYER},
};

enum struct PlayerState {
    STAND = 0,
    RUN_DOWN = 1,
    RUN_RIGHT = 2,
    RUN_UP = 3,
    RUN_LEFT = 4
};

using EntityID = uint32_t;

struct CParent
{
    EntityID parent;
    Vec2 relativePos = {0,0};
    CParent() {}
    CParent(EntityID p) : parent(p){}
    CParent(EntityID p, Vec2 relPos) : parent(p), relativePos(relPos) {}
};

struct CProjectile
{
    EntityID projectileID;
    CProjectile() {}
    CProjectile(EntityID p) : projectileID(p){}
};

struct CInput
{
    Vec2 direction = {0, 0};
    bool up         = false;
    bool down       = false;
    bool left       = false;
    bool right      = false;
    bool shift      = false;
    bool ctrl       = false;
    bool interact   = false;
    bool attack     = false;

    bool shoot      = false;
    bool canShoot   = false;
    bool posses     = false;
    CInput() {};
};

struct CTransform
{
    Vec2 pos = {0, 0};  
    Vec2 prevPos = {0, 0};  
    Vec2 scale = {1, 1};    
    float angle = 0;
    CTransform() {}
    CTransform(const Vec2 & p)
        : pos(p), prevPos(p) {}
    CTransform(const Vec2 & p, float a)
        : pos(p), prevPos(p), angle(a) {}
    CTransform(const Vec2 & p, float a, Vec2 s)
        : pos(p), prevPos(p), angle(a), scale(s){}
    CTransform(const Vec2 & p, Vec2 s)
        : pos(p), prevPos(p), scale(s){}
    CTransform(const json j)
        : pos(j["pos"]), prevPos(j["pos"]) {}
};

struct CVelocity
{
    Vec2 vel = {0, 0};    
    float speed = 100;
    float tempo = 1.0f;
    CVelocity() {}
    CVelocity(const float s) 
        : speed(s){}
    CVelocity(const json j)
        : speed(j["speed"]) {}
    CVelocity(Vec2 v, const float s) 
        : vel(v), speed(s){}
};

struct CBox 
{
    Vec2 size;
    Vec2 halfSize;
    Vec2 offset;
    CollisionMask layer = EMPTY_MASK;
    CollisionMask mask = EMPTY_MASK; // bitmask of layers this entity should collide with
    Color color = {255, 255, 255, 255};

    CBox() {}
    CBox(const Vec2& s) 
        : size(s), halfSize(s/2.0), color({255, 255, 255, 255}) {}
    CBox(const Vec2& s, CollisionMask l, CollisionMask m, Color c) // only use this after the new collision system is implemented
        : size(s), halfSize(s/2.0), layer(l), mask(m), color(c) {}
    CBox(const Vec2& s, Color c) // only use this after the new collision system is implemented
        : size(s), halfSize(s/2.0), color(c) {}
    CBox(json j, Color c){
        color = c;
        size = j["size"];
        halfSize = size/2;
        if (j.contains("color")){
            color = Color{
                static_cast<uint8_t>(j["color"]["r"].get<int>()),
                static_cast<uint8_t>(j["color"]["g"].get<int>()),
                static_cast<uint8_t>(j["color"]["b"].get<int>()),
                static_cast<uint8_t>(j["color"]["a"].get<int>())
            };
        }
        if (j.contains("layer")){
            layer = componentMaskMap[j["layer"]];
        }
        if (j.contains("mask")){
            mask = EMPTY_MASK;
            for (const auto& maskStr : j["mask"]) {
                mask = mask | componentMaskMap.at(maskStr.get<std::string>());
            }
        }
    }
};

struct CCollisionBox : public CBox
{
    CCollisionBox() {}
    CCollisionBox(const Vec2& s) 
        : CBox(s) {}
    CCollisionBox(const Vec2& s, const Color color) 
        : CBox(s, color) {}

    CCollisionBox(const Vec2& size, CollisionMask layer, CollisionMask mask) // only use this after the new collision system is implemented
        : CBox(size, layer, mask, {255, 255, 255, 255}) {}
    CCollisionBox(json j) 
        : CBox(j, {255, 255, 255, 255}) {}    
};

struct CInteractionBox : public CBox
{
    CInteractionBox(){}    
    CInteractionBox(const Vec2& size, CollisionMask layer, CollisionMask mask) // only use this after the new collision system is implemented
        : CBox(size, layer, mask, {0, 0, 255, 255}) {}
    CInteractionBox(json j) 
        : CBox(j, {0, 0, 255, 255}) {} 
};

struct CImmovable
{
    CImmovable(){}    
};

struct CWater {
    bool isDeep = false;  // Differentiates deep vs shallow water
    CWater() {}
    CWater(bool d) 
        : isDeep(d) {}
};

struct CCurrency
{
    int value = 1;
    CCurrency() {}    
    CCurrency(const json j) 
        : value(j["value"]){}    
};

struct CHealth
{
    int HP = 6;
    int HP_max = 6;
    int i_frames = 30;
    size_t damage_frame = 0;
    std::unordered_set<std::string> HPType;
    CHealth() {}
    CHealth(int hp, int hp_max, int hrt_frms)
        : HP(hp), HP_max(hp_max), i_frames(hrt_frms){}
    CHealth(const json& j) {
        HP       = j["HP"];
        HP_max   = j["HP_max"];
        i_frames = j["i_frames"];
        if (j.contains("type"))
            for (auto& t : j["type"])
            HPType.insert(t);
    }
};

struct CLifespan
{
    int lifespan;
    CLifespan() {}
    CLifespan(int lf) : lifespan(lf){}
};

struct CSprite
{
    TextureHandle texture;
    RectF src;
    int layer = 0;
    bool visible = true;

    CSprite() {}
    CSprite(const SpriteDefinition& sprite, int l)
        : texture(sprite.texture()), src(sprite.firstFrame()), layer(l) {}

    Vec2 size() const {
        return Vec2{src.w, src.h};
    }
};

struct CAnimation
{
    size_t frameCount = 1;
    size_t currentFrame = 0;
    size_t frameDuration = 1;
    Vec2 frameSize = {1, 1};
    Vec2 sourceOrigin = {0, 0};
    int cols = 1;
    int currentRow = 0;
    int currentCol = 0;
    bool repeat = true;

    CAnimation() {}
    CAnimation(const SpriteDefinition& sprite, bool r = true)
        : frameCount(std::max<size_t>(1, sprite.frameCount())),
          frameDuration(std::max<size_t>(1, sprite.frameDuration())),
          frameSize(sprite.frameSize()),
          sourceOrigin(Vec2{sprite.sourceRegion().x, sprite.sourceRegion().y}),
          cols(std::max(1, sprite.cols())),
          repeat(r) {}

    bool hasEnded() const {
        return (currentFrame / frameDuration) % frameCount == frameCount - 1;
    }

    RectF sourceRect() const {
        const size_t frame = (currentFrame / frameDuration) % frameCount;
        int step = static_cast<int>(cols / static_cast<int>(frameCount));
        step = std::max(1, step);
        const int col = std::min(cols - 1, currentCol + static_cast<int>(frame) * step);
        return RectF{
            sourceOrigin.x + col * frameSize.x,
            sourceOrigin.y + currentRow * frameSize.y,
            frameSize.x,
            frameSize.y
        };
    }
};  

struct CAudio
{
    std::string audioName;
    int loops = 0;
    CAudio(std::string a, int l)
        : audioName(a), loops(l){}
    CAudio(std::string a)
        : audioName(a){}
};

struct CState
{
    PlayerState state = PlayerState::STAND;
    PlayerState preState = PlayerState::STAND; 
    bool changeAnimate = true;
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

struct CAttack
{
    int damage = 1;
    int speed = 120;
    int attackTimer = 120;
    int duration = 30;
    int range = 3*64;
    Vec2 area = {16, 16};
    CAttack() {}
    CAttack(int dmg, int spd, int dur, int rg, Vec2 ae) 
        : damage(dmg), speed(spd), attackTimer(spd), duration(dur), range(rg), area(ae){}
    CAttack(json j){
        damage      = j["damage"];
        speed       = j["speed"];
        attackTimer = j["speed"];
        duration    = j["duration"];
        range       = j["range"];
        area        = j["area"];
    }
};

struct CDamage
{
    int damage;
    std::unordered_set<std::string> damageType;
    CDamage() {}
    CDamage(int dmg) : damage(dmg) {}
    CDamage(int dmg, std::unordered_set<std::string> dmgType) 
        : damage(dmg), damageType(dmgType) {}
};

struct CText
{    
    Vec2 size;
    std::string text;
    std::string font_name;

    CText() {}
    CText(std::string txt, const float sz, std::string fnt)
        : text(txt), size(Vec2{sz*txt.length()/4, sz}), font_name(fnt){}
};

struct CPossesLevel
{
    int level = 10;
    CPossesLevel() {}
    CPossesLevel(int l)
        : level(l) {}
    CPossesLevel(const json& j) {
        if (j.is_number()) {
            level = j;
        }
        else if (j.contains("level")) {
            level = j["level"];
        }
    }
};

struct CFollow
{
    std::string target;
    CFollow() {}
    CFollow(json j){
        target = j.get<std::string>();
    }
};

struct CPath
{
    std::vector<Vec2> path;
    int index = 0;
    CPath() {}
    CPath( std::vector<Vec2> p)
        : path(p){}
    CPath(json j){
        path = j.get<std::vector<Vec2>>();
    }
};

enum class AIStateType {
    Patrol,
    Chase,
    Investigate
};

struct CAIAgent {
    // --- Sight ---
    float sightRange   = 200.0f;
    bool  canSeePlayer = false;

    // --- Memory ---
    Vec2  lastKnownPlayerPos = {0, 0};
    int   memoryTimer        = 0;
    int   memoryDuration     = 240;   // frames before giving up investigation

    // --- Patrol ---
    Vec2  spawnPos           = {0, 0};
    float patrolRadius       = 96.0f;
    Vec2  patrolTarget       = {0, 0};
    bool  hasPatrolTarget    = false;
    int   patrolWaitTimer    = 0;
    int   patrolWaitDuration = 90;    // frames to stand still between patrol points

    // --- State ---
    AIStateType state = AIStateType::Patrol;

    CAIAgent() {}
    CAIAgent(const json& j) {
        sightRange       = j.value("sightRange",       200.0f);
        patrolRadius     = j.value("patrolRadius",     96.0f);
        memoryDuration   = j.value("memoryDuration",   240);
        patrolWaitDuration = j.value("patrolWaitDuration", 90);
    }
};

struct CItem{
    int itemID;
    CItem(int id)
        : itemID(id) {}
};

struct CInventory{
    Item activeItem;
    std::array<Item, 3> items;
    CInventory() {
        activeItem.index = 0;
        int index = 0;
        for (Item& item: items){
            item.index = index;
            index++;
        }
    }
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

enum struct WeaponType {
    Melee,
    Projectile,
    AoE
};

struct CWeapon
{
    int damage = 1;
    int speed = 600;
    int delay = 0;
    int range = 180;
    WeaponType weaponType = WeaponType::Projectile;

    CWeapon() {}
    CWeapon(int damage, int speed, int range)
                : damage(damage), speed(speed), range(range){}
    
    CWeapon(int damage, int speed, int range, WeaponType type)
                : damage(damage), speed(speed), range(range), weaponType(type){}

    CWeapon(const json& j) {
        damage = j["damage"];
        speed  = j["speed"];
        delay  = j["speed"]; // intentional
        range  = j["range"];
        static const std::unordered_map<
            std::string, WeaponType> wMap = {
            {"Melee",      WeaponType::Melee},
            {"Projectile", WeaponType::Projectile},
            {"AoE",        WeaponType::AoE}
        };
        weaponType = wMap.at(j["type"]);
    }
};

struct CEvent
{
    int questID;
    Event event;

    CEvent() {}
    CEvent(Event e)
            : event(e){}
};

struct CChild
{

    std::vector<std::tuple<EntityID, bool>> children;

    EntityID childID;
    bool removeOnDeath;
    CChild() {}
    CChild(EntityID cID)
                : childID(cID), removeOnDeath(true){}
    CChild(EntityID cID, bool remove)
                : childID(cID), removeOnDeath(remove){}
};

struct CChunk
{
    Vec2 chunkPos;
    std::vector<EntityID> chunkChildern;
    CChunk(Vec2 cPos)
            : chunkPos(cPos){}
    CChunk() {}
};
