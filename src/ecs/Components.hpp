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
#include <stdexcept>
#include <vector>
#include <string>

using json = nlohmann::json;

constexpr uint8_t MAX_LAYERS = 16;

namespace RenderLayer {
constexpr int ReservedBelowTerrain = 1;
constexpr int TerrainTilesLow = 2;
constexpr int TerrainTilesHigh = 3;
constexpr int GroundShadow = 4;
constexpr int CharacterShadow = 5;
constexpr int PropShadow = 6;
constexpr int Character = 7;
constexpr int GroundItem = 8;
constexpr int WorldProp = 9;
constexpr int Projectile = 10;
constexpr int AttackEffect = 11;
constexpr int Dialog = 12;

constexpr int MenuBackground = 1;
constexpr int MenuTitle = 2;
constexpr int MenuControl = 3;
}

inline const std::unordered_map<std::string, int> renderLayerMap =
{
    {"RESERVED_BELOW_TERRAIN_RENDER_LAYER", RenderLayer::ReservedBelowTerrain},
    {"TERRAIN_TILE_LOW_RENDER_LAYER", RenderLayer::TerrainTilesLow},
    {"TERRAIN_TILE_HIGH_RENDER_LAYER", RenderLayer::TerrainTilesHigh},
    {"GROUND_SHADOW_RENDER_LAYER", RenderLayer::GroundShadow},
    {"GROUND_ITEM_RENDER_LAYER", RenderLayer::GroundItem},
    {"PROP_SHADOW_RENDER_LAYER", RenderLayer::PropShadow},
    {"WORLD_PROP_RENDER_LAYER", RenderLayer::WorldProp},
    {"CHARACTER_SHADOW_RENDER_LAYER", RenderLayer::CharacterShadow},
    {"CHARACTER_RENDER_LAYER", RenderLayer::Character},
    {"PROJECTILE_RENDER_LAYER", RenderLayer::Projectile},
    {"ATTACK_EFFECT_RENDER_LAYER", RenderLayer::AttackEffect},
    {"DIALOG_RENDER_LAYER", RenderLayer::Dialog},
    {"MENU_BACKGROUND_RENDER_LAYER", RenderLayer::MenuBackground},
    {"MENU_TITLE_RENDER_LAYER", RenderLayer::MenuTitle},
    {"MENU_CONTROL_RENDER_LAYER", RenderLayer::MenuControl},
};

inline int renderLayerFromJson(const json& value)
{
    if (value.is_number_integer()) {
        return value.get<int>();
    }

    if (value.is_string()) {
        const std::string layerName = value.get<std::string>();
        const auto it = renderLayerMap.find(layerName);
        if (it != renderLayerMap.end()) {
            return it->second;
        }
        throw std::out_of_range("Unknown render layer: " + layerName);
    }

    throw std::invalid_argument("Render layer must be an integer or render layer name");
}

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
    EntityID owner = 0;
    Vec2 direction = {1, 0};
    float speed = 200.0f;
    int flightLifetime = 60;
    float createOffset = 12.0f;
    CollisionMask targetMask = ENEMY_LAYER;
    CProjectile() {}
    CProjectile(EntityID projectileOwner, Vec2 projectileDirection)
        : owner(projectileOwner), direction(projectileDirection) {}
    CProjectile(
        EntityID projectileOwner,
        Vec2 projectileDirection,
        float projectileSpeed,
        int lifetime,
        float offset,
        CollisionMask targets = ENEMY_LAYER
    )
        : owner(projectileOwner),
          direction(projectileDirection),
          speed(projectileSpeed),
          flightLifetime(lifetime),
          createOffset(offset),
          targetMask(targets) {}
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
    bool use        = false;
    bool useHeld    = false;

    bool shoot      = false;
    bool canShoot   = false;
    bool posses     = false;
    bool possesHeld     = false;
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
    // World-space linear velocity in pixels per second.
    Vec2 vel = {0, 0};
    CVelocity() {}
    CVelocity(const Vec2& v)
        : vel(v) {}
};

struct CPhysicsBody
{
    float mass = 1.0f;
    float moveForce = 2400.0f;
    float maxSpeed = 100.0f;
    float linearDamping = 24.0f;
    Vec2 accumulatedForce = {0, 0};

    CPhysicsBody() = default;

    CPhysicsBody(float bodyMass, float force, float speed, float damping)
        : mass(bodyMass), moveForce(force), maxSpeed(speed), linearDamping(damping)
    {
        validate();
    }

    CPhysicsBody(const json& j)
        : mass(j.at("mass").get<float>()),
          moveForce(j.at("moveForce").get<float>()),
          maxSpeed(j.at("maxSpeed").get<float>()),
          linearDamping(j.at("linearDamping").get<float>())
    {
        validate();
    }

private:
    void validate() const
    {
        if (mass <= 0.0f || moveForce < 0.0f || maxSpeed < 0.0f || linearDamping < 0.0f) {
            throw std::invalid_argument("CPhysicsBody requires positive mass and non-negative tuning values");
        }
    }
};

inline CollisionMask collisionMaskFromJson(const json& value)
{
    if (value.is_string()) {
        return componentMaskMap.at(value.get<std::string>());
    }

    CollisionMask mask = EMPTY_MASK;
    for (const auto& maskStr : value) {
        mask |= componentMaskMap.at(maskStr.get<std::string>());
    }
    return mask;
}

inline Color colorFromJson(const json& value)
{
    return Color{
        static_cast<uint8_t>(value.at("r").get<int>()),
        static_cast<uint8_t>(value.at("g").get<int>()),
        static_cast<uint8_t>(value.at("b").get<int>()),
        static_cast<uint8_t>(value.at("a").get<int>())
    };
}

struct ColliderShape
{
    Vec2 offset = {0, 0};
    Vec2 size = {0, 0};
    Vec2 halfSize = {0, 0};
    CollisionMask layer = EMPTY_MASK;
    CollisionMask targetMask = EMPTY_MASK;
    Color debugColor = {255, 255, 255, 255};
    bool isTrigger = false;

    ColliderShape() = default;
    ColliderShape(const Vec2& s)
        : size(s), halfSize(s / 2.0f) {}
    ColliderShape(const Vec2& s, CollisionMask l, CollisionMask targets, bool trigger = false)
        : size(s), halfSize(s / 2.0f), layer(l), targetMask(targets), isTrigger(trigger) {}
    ColliderShape(
        const Vec2& shapeOffset,
        const Vec2& shapeSize,
        CollisionMask shapeLayer,
        CollisionMask shapeTargets,
        Color shapeDebugColor,
        bool trigger
    )
        : offset(shapeOffset),
          size(shapeSize),
          halfSize(shapeSize / 2.0f),
          layer(shapeLayer),
          targetMask(shapeTargets),
          debugColor(shapeDebugColor),
          isTrigger(trigger) {}
    ColliderShape(const json& j, Color defaultColor = {255, 255, 255, 255}, bool defaultTrigger = false)
    {
        offset = j.value("offset", Vec2{0, 0});
        size = j.at("size").get<Vec2>();
        halfSize = size / 2.0f;
        debugColor = j.contains("debugColor") ? colorFromJson(j.at("debugColor")) : defaultColor;
        isTrigger = j.value("isTrigger", defaultTrigger);
        if (j.contains("layer")) {
            layer = collisionMaskFromJson(j.at("layer"));
        }
        if (j.contains("targetMask")) {
            targetMask = collisionMaskFromJson(j.at("targetMask"));
        }
    }
};

struct CCollider
{
    std::vector<ColliderShape> shapes;

    CCollider() = default;
    CCollider(const Vec2& size)
        : shapes{ColliderShape(size)} {}
    CCollider(const Vec2& size, CollisionMask layer, CollisionMask targetMask, bool isTrigger = false)
        : shapes{ColliderShape(size, layer, targetMask, isTrigger)} {}
    CCollider(const Vec2& size, CollisionMask layer, CollisionMask targetMask, Color debugColor, bool isTrigger = false)
        : shapes{ColliderShape(Vec2{0, 0}, size, layer, targetMask, debugColor, isTrigger)} {}
    CCollider(std::vector<ColliderShape> colliderShapes)
        : shapes(std::move(colliderShapes)) {}
    CCollider(const json& j)
    {
        for (const auto& shapeJson : j.at("shapes")) {
            shapes.emplace_back(shapeJson);
        }
    }

    void addShape(const ColliderShape& shape)
    {
        shapes.push_back(shape);
    }
};

struct CWater {
    bool isDeep = false;  // Differentiates deep vs shallow water
    CWater() {}
    CWater(bool d) 
        : isDeep(d) {}
};

struct CSwimming
{
    EntityID childEntity = 0;

    CSwimming() = default;
    explicit CSwimming(EntityID child)
        : childEntity(child) {}
};

struct CCurrency
{
    int value = 0;
    CCurrency() {}
    CCurrency(const json& j)
        : value(j.value("value", 0)){}
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

struct CDamageFlash
{
    int framesRemaining = 8;
    int totalFrames = 8;

    CDamageFlash() {}
    CDamageFlash(int frames)
        : framesRemaining(frames), totalFrames(frames) {}

    void reset() {
        framesRemaining = totalFrames;
    }

    float whiteTint() const {
        if (totalFrames <= 0 || framesRemaining <= 0) {
            return 0.0f;
        }
        return std::clamp(
            static_cast<float>(framesRemaining) / static_cast<float>(totalFrames),
            0.0f,
            1.0f
        );
    }
};

struct CLifespan
{
    int lifespan;
    CLifespan() {}
    CLifespan(int lf) : lifespan(lf){}
};

struct CActiveHitboxLifetime
{
    int framesRemaining = 15;

    CActiveHitboxLifetime() {}
    CActiveHitboxLifetime(int frames)
        : framesRemaining(frames) {}
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

enum class ProjectilePhase {
    Flying,
    Destroying
};

struct CProjectileState
{
    ProjectilePhase phase = ProjectilePhase::Flying;
    CProjectileState() {}
    CProjectileState(ProjectilePhase projectilePhase) : phase(projectilePhase) {}
}; 
struct CName
{
    std::string name;
    CName() {}
    CName(const std::string nm) : name(nm) {}
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

struct CAttackHitbox
{
    EntityID owner = 0;
    std::unordered_set<EntityID> hitEntities;

    CAttackHitbox() {}
    CAttackHitbox(EntityID ownerID)
        : owner(ownerID) {}
};

struct CAttackState
{
    Vec2 direction = {1, 0};
    int attackHitFrame = 0;
    int animationFrameCount = 1;
    int animationFrameDuration = 1;
    int elapsedFrames = 0;
    bool hasFired = false;
    bool hasAnimationOverride = false;
    bool hadAnimation = false;
    CSprite previousSprite;
    CAnimation previousAnimation;

    CAttackState() {}
    CAttackState(Vec2 attackDirection)
        : direction(attackDirection) {}

    int hitGameFrame() const {
        return std::max(0, attackHitFrame * animationFrameDuration);
    }

    int totalGameFrames() const {
        return std::max(1, animationFrameCount * animationFrameDuration);
    }

    int finishGameFrame() const {
        return std::max(1, totalGameFrames() - 1);
    }
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

enum class PossessState {
    Drain,
    Possess
};

struct CPossessable
{
    int level = 10;
    int duration = 120;
    int timeLeft = 120;
    int lifeForce;
    PossessState state = PossessState::Drain;

    CPossessable() {}
    CPossessable(int l)
        : level(l) {}
    CPossessable(const json& j) {
        if (j.is_number()) {
            level = j;
        }
        else if (j.contains("level")) {
            level = j["level"];
        }
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
    bool hasPickupModeOverride = false;
    PickupMode pickupModeOverride = PickupMode::Manual;

    CItem(int id)
        : itemID(id) {}
    CItem(int id, PickupMode mode)
        : itemID(id), hasPickupModeOverride(true), pickupModeOverride(mode) {}
};

struct CInventory{
    static constexpr int DefaultSlotCount = 3;

    Item activeItem;
    std::vector<Item> items;

    int size() const {
        return static_cast<int>(items.size());
    }

    CInventory(int slotCount = DefaultSlotCount)
        : items(std::max(1, slotCount)) {
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
    std::string attackAnimation;
    int attackAnimationRow = -1;
    int attackHitFrame = 0;
    std::string hitboxSprite = "sword";
    std::string hitboxAnimation;
    int hitboxActiveFrames = 15;
    WeaponType weaponType = WeaponType::Projectile;
    CollisionMask targetMask = ENEMY_LAYER;

    CWeapon() {}
    CWeapon(int damage, int speed, int range)
                : damage(damage), speed(speed), range(range){}
    
    CWeapon(int damage, int speed, int range, WeaponType type, CollisionMask mask = ENEMY_LAYER)
                : damage(damage), speed(speed), range(range), weaponType(type), targetMask(mask){}

    CWeapon(const json& j) {
        damage = j["damage"];
        speed  = j["speed"];
        delay  = j["speed"]; // intentional
        range  = j["range"];
        attackAnimation = j.value("attackAnimation", "");
        attackAnimationRow = j.value("attackAnimationRow", attackAnimationRow);
        attackHitFrame = j.value("attackHitFrame", attackHitFrame);
        hitboxSprite = j.value("hitboxSprite", hitboxSprite);
        hitboxAnimation = j.value("hitboxAnimation", "");
        hitboxActiveFrames = j.value("hitboxActiveFrames", hitboxActiveFrames);
        static const std::unordered_map<
            std::string, WeaponType> wMap = {
            {"Melee",      WeaponType::Melee},
            {"Projectile", WeaponType::Projectile},
            {"AoE",        WeaponType::AoE}
        };
        weaponType = wMap.at(j["type"]);
        if (j.contains("mask")) {
            targetMask = EMPTY_MASK;
            for (const auto& maskStr : j["mask"]) {
                targetMask = targetMask | componentMaskMap.at(maskStr.get<std::string>());
            }
        }
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

struct ChildLink
{
    EntityID child = 0;
    bool removeOnDeath = true;

    ChildLink() {}
    ChildLink(EntityID childID, bool remove)
        : child(childID), removeOnDeath(remove) {}
};

struct CChild
{
    std::vector<ChildLink> children;

    CChild() {}
    CChild(EntityID cID)
        : children{{cID, true}} {}
    CChild(EntityID cID, bool remove)
        : children{{cID, remove}} {}
};

struct CStatic
{
};

struct CChunk
{
};
