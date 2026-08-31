#include "debug/EntityInspector.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

namespace {

using json = nlohmann::json;

json vec2Json(const Vec2& value)
{
    return {{"x", value.x}, {"y", value.y}};
}

json rectJson(const RectF& value)
{
    return {{"x", value.x}, {"y", value.y}, {"w", value.w}, {"h", value.h}};
}

json colorJson(const Color& value)
{
    return {{"r", value.r}, {"g", value.g}, {"b", value.b}, {"a", value.a}};
}

std::string factionName(Faction faction)
{
    switch (faction) {
    case Faction::Demon: return "Demon";
    case Faction::Enemy: return "Enemy";
    case Faction::Wizard: return "Wizard";
    case Faction::Dwarf: return "Dwarf";
    case Faction::Elf: return "Elf";
    case Faction::Knight: return "Knight";
    case Faction::Neutral: return "Neutral";
    }
    return "Unknown";
}

std::string playerStateName(PlayerState state)
{
    switch (state) {
    case PlayerState::STAND: return "STAND";
    case PlayerState::RUN_DOWN: return "RUN_DOWN";
    case PlayerState::RUN_RIGHT: return "RUN_RIGHT";
    case PlayerState::RUN_UP: return "RUN_UP";
    case PlayerState::RUN_LEFT: return "RUN_LEFT";
    }
    return "Unknown";
}

std::string projectilePhaseName(ProjectilePhase phase)
{
    switch (phase) {
    case ProjectilePhase::Flying: return "Flying";
    case ProjectilePhase::Destroying: return "Destroying";
    }
    return "Unknown";
}

std::string possessStateName(PossessState state)
{
    switch (state) {
    case PossessState::Drain: return "Drain";
    case PossessState::Possess: return "Possess";
    }
    return "Unknown";
}

std::string aiStateName(AIStateType state)
{
    switch (state) {
    case AIStateType::Patrol: return "Patrol";
    case AIStateType::Chase: return "Chase";
    case AIStateType::Investigate: return "Investigate";
    }
    return "Unknown";
}

std::string weaponTypeName(WeaponType type)
{
    switch (type) {
    case WeaponType::Melee: return "Melee";
    case WeaponType::Projectile: return "Projectile";
    case WeaponType::AoE: return "AoE";
    }
    return "Unknown";
}

std::string itemTypeName(ItemType type)
{
    switch (type) {
    case ItemType::None: return "None";
    case ItemType::Weapon: return "Weapon";
    case ItemType::WeaponMelee: return "WeaponMelee";
    case ItemType::WeaponRanged: return "WeaponRanged";
    case ItemType::WeaponAoE: return "WeaponAoE";
    case ItemType::Consumable: return "Consumable";
    case ItemType::Quest: return "Quest";
    case ItemType::Currency: return "Currency";
    }
    return "Unknown";
}

std::string pickupModeName(PickupMode mode)
{
    switch (mode) {
    case PickupMode::Manual: return "Manual";
    case PickupMode::Automatic: return "Automatic";
    }
    return "Unknown";
}

std::string eventTypeName(EventType type)
{
    switch (type) {
    case EventType::ItemPickedUp: return "ItemPickedUp";
    case EventType::EnteredArea: return "EnteredArea";
    case EventType::EntityKilled: return "EntityKilled";
    case EventType::EntitySpawned: return "EntitySpawned";
    case EventType::DialogueFinished: return "DialogueFinished";
    case EventType::FlagChanged: return "FlagChanged";
    case EventType::EntityDrained: return "EntityDrained";
    case EventType::EntityPossessed: return "EntityPossessed";
    case EventType::NoEvent: return "NoEvent";
    }
    return "Unknown";
}

json collisionMaskJson(const CollisionMask& mask)
{
    static constexpr std::array<std::pair<const char*, CollisionMask>, 13> layers = {{
        {"PLAYER_LAYER", PLAYER_LAYER},
        {"ENEMY_LAYER", ENEMY_LAYER},
        {"PROJECTILE_LAYER", PROJECTILE_LAYER},
        {"OBSTACLE_LAYER", OBSTACLE_LAYER},
        {"FRIENDLY_LAYER", FRIENDLY_LAYER},
        {"DAMAGE_LAYER", DAMAGE_LAYER},
        {"WATER_LAYER", WATER_LAYER},
        {"LOOT_LAYER", LOOT_LAYER},
        {"AREA_LAYER", AREA_LAYER},
        {"WIZARD_LAYER", WIZARD_LAYER},
        {"DWARF_LAYER", DWARF_LAYER},
        {"ELF_LAYER", ELF_LAYER},
        {"KNIGHT_LAYER", KNIGHT_LAYER}
    }};

    json layerNames = json::array();
    for (const auto& [name, layer] : layers) {
        if ((mask & layer) == layer) {
            layerNames.push_back(name);
        }
    }
    return {{"bits", mask.to_ulong()}, {"layers", layerNames}};
}

template <typename T>
json sortedJsonArray(const std::unordered_set<T>& values)
{
    std::vector<T> sorted(values.begin(), values.end());
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

json colliderShapeJson(const ColliderShape& shape)
{
    return {
        {"offset", vec2Json(shape.offset)},
        {"size", vec2Json(shape.size)},
        {"halfSize", vec2Json(shape.halfSize)},
        {"layer", collisionMaskJson(shape.layer)},
        {"targetMask", collisionMaskJson(shape.targetMask)},
        {"debugColor", colorJson(shape.debugColor)},
        {"isTrigger", shape.isTrigger}
    };
}

json spriteJson(const CSprite& sprite)
{
    return {
        {"texture", sprite.texture.name},
        {"src", rectJson(sprite.src)},
        {"layer", sprite.layer},
        {"visible", sprite.visible}
    };
}

json animationJson(const CAnimation& animation)
{
    return {
        {"frameCount", animation.frameCount},
        {"currentFrame", animation.currentFrame},
        {"frameDuration", animation.frameDuration},
        {"frameSize", vec2Json(animation.frameSize)},
        {"sourceOrigin", vec2Json(animation.sourceOrigin)},
        {"cols", animation.cols},
        {"currentRow", animation.currentRow},
        {"currentCol", animation.currentCol},
        {"repeat", animation.repeat}
    };
}

json itemJson(const Item& item)
{
    return {
        {"id", item.id},
        {"index", item.index},
        {"name", item.name},
        {"description", item.description},
        {"iconPath", item.iconPath},
        {"stack", item.stack},
        {"maxStack", item.maxStack},
        {"pickupMode", pickupModeName(item.pickupMode)},
        {"currencyValue", item.currencyValue},
        {"type", itemTypeName(item.type)},
        {"damage", item.damage},
        {"healing", item.healing},
        {"hasWeaponConfig", item.hasWeaponConfig},
        {"weaponConfig", item.weaponConfig},
        {"hasShadowConfig", item.hasShadowConfig},
        {"shadowConfig", item.shadowConfig}
    };
}

template <typename Component, typename Serializer>
void appendComponentIfPresent(
    const ECS& ecs,
    EntityID entity,
    json& components,
    const char* name,
    Serializer serializer)
{
    if (ecs.hasComponent<Component>(entity)) {
        components[name] = serializer(ecs.getComponent<Component>(entity));
    }
}

bool pointInCollider(const Vec2& point, const CTransform& transform, const CCollider& collider)
{
    for (const ColliderShape& shape : collider.shapes) {
        const Vec2 center = transform.pos + shape.offset;
        const Vec2 halfSize = shape.size / 2.0f;
        if (point.x >= center.x - halfSize.x && point.x <= center.x + halfSize.x &&
            point.y >= center.y - halfSize.y && point.y <= center.y + halfSize.y) {
            return true;
        }
    }
    return false;
}

} // namespace

namespace DebugEntityInspector {

json inspectEntity(const ECS& ecs, EntityID entity)
{
    json result = {
        {"entityId", entity},
        {"alive", ecs.isAlive(entity)},
        {"components", json::object()}
    };
    if (!ecs.isAlive(entity)) {
        return result;
    }

    if (ecs.hasComponent<CName>(entity)) {
        result["name"] = ecs.getComponent<CName>(entity).name;
    }

    json& components = result["components"];
    appendComponentIfPresent<CParent>(ecs, entity, components, "CParent", [](const CParent& value) {
        return json{{"parent", value.parent}, {"relativePos", vec2Json(value.relativePos)}};
    });
    appendComponentIfPresent<CProjectile>(ecs, entity, components, "CProjectile", [](const CProjectile& value) {
        return json{{"owner", value.owner}, {"direction", vec2Json(value.direction)}, {"speed", value.speed},
                    {"flightLifetime", value.flightLifetime}, {"createOffset", value.createOffset},
                    {"targetMask", collisionMaskJson(value.targetMask)}};
    });
    appendComponentIfPresent<CInput>(ecs, entity, components, "CInput", [](const CInput& value) {
        return json{{"direction", vec2Json(value.direction)}, {"up", value.up}, {"down", value.down},
                    {"left", value.left}, {"right", value.right}, {"shift", value.shift}, {"ctrl", value.ctrl},
                    {"interact", value.interact}, {"use", value.use}, {"useHeld", value.useHeld},
                    {"shoot", value.shoot}, {"canShoot", value.canShoot}, {"posses", value.posses},
                    {"possesHeld", value.possesHeld}};
    });
    appendComponentIfPresent<CTransform>(ecs, entity, components, "CTransform", [](const CTransform& value) {
        return json{{"pos", vec2Json(value.pos)}, {"prevPos", vec2Json(value.prevPos)},
                    {"scale", vec2Json(value.scale)}, {"angle", value.angle}};
    });
    appendComponentIfPresent<CShadow>(ecs, entity, components, "CShadow", [](const CShadow& value) {
        return json{{"scale", value.scale}, {"offset", vec2Json(value.offset)}};
    });
    appendComponentIfPresent<CVelocity>(ecs, entity, components, "CVelocity", [](const CVelocity& value) {
        return json{{"vel", vec2Json(value.vel)}};
    });
    appendComponentIfPresent<CPhysicsBody>(ecs, entity, components, "CPhysicsBody", [](const CPhysicsBody& value) {
        return json{{"mass", value.mass}, {"moveForce", value.moveForce}, {"maxSpeed", value.maxSpeed},
                    {"linearDamping", value.linearDamping}, {"accumulatedForce", vec2Json(value.accumulatedForce)}};
    });
    appendComponentIfPresent<CCollider>(ecs, entity, components, "CCollider", [](const CCollider& value) {
        json shapes = json::array();
        for (const ColliderShape& shape : value.shapes) {
            shapes.push_back(colliderShapeJson(shape));
        }
        return json{{"shapes", shapes}};
    });
    appendComponentIfPresent<CAllegiance>(ecs, entity, components, "CAllegiance", [](const CAllegiance& value) {
        return json{{"trueFaction", factionName(value.trueFaction)}, {"perceivedFaction", factionName(value.perceivedFaction)}};
    });
    appendComponentIfPresent<CWater>(ecs, entity, components, "CWater", [](const CWater& value) {
        return json{{"isDeep", value.isDeep}};
    });
    appendComponentIfPresent<CSwimming>(ecs, entity, components, "CSwimming", [](const CSwimming& value) {
        return json{{"childEntity", value.childEntity}};
    });
    appendComponentIfPresent<CCurrency>(ecs, entity, components, "CCurrency", [](const CCurrency& value) {
        return json{{"value", value.value}};
    });
    appendComponentIfPresent<CHealth>(ecs, entity, components, "CHealth", [](const CHealth& value) {
        return json{{"HP", value.HP}, {"HP_max", value.HP_max}, {"i_frames", value.i_frames},
                    {"damage_frame", value.damage_frame}, {"HPType", sortedJsonArray(value.HPType)}};
    });
    appendComponentIfPresent<CDamageFlash>(ecs, entity, components, "CDamageFlash", [](const CDamageFlash& value) {
        return json{{"framesRemaining", value.framesRemaining}, {"totalFrames", value.totalFrames}};
    });
    appendComponentIfPresent<CLifespan>(ecs, entity, components, "CLifespan", [](const CLifespan& value) {
        return json{{"lifespan", value.lifespan}};
    });
    appendComponentIfPresent<CActiveHitboxLifetime>(ecs, entity, components, "CActiveHitboxLifetime", [](const CActiveHitboxLifetime& value) {
        return json{{"framesRemaining", value.framesRemaining}};
    });
    appendComponentIfPresent<CSprite>(ecs, entity, components, "CSprite", spriteJson);
    appendComponentIfPresent<CAnimation>(ecs, entity, components, "CAnimation", animationJson);
    appendComponentIfPresent<CAudio>(ecs, entity, components, "CAudio", [](const CAudio& value) {
        return json{{"audioName", value.audioName}, {"loops", value.loops}};
    });
    appendComponentIfPresent<CState>(ecs, entity, components, "CState", [](const CState& value) {
        return json{{"state", playerStateName(value.state)}, {"preState", playerStateName(value.preState)},
                    {"facing", playerStateName(value.facing)}, {"changeAnimate", value.changeAnimate}};
    });
    appendComponentIfPresent<CProjectileState>(ecs, entity, components, "CProjectileState", [](const CProjectileState& value) {
        return json{{"phase", projectilePhaseName(value.phase)}};
    });
    appendComponentIfPresent<CName>(ecs, entity, components, "CName", [](const CName& value) {
        return json{{"name", value.name}};
    });
    appendComponentIfPresent<CDamage>(ecs, entity, components, "CDamage", [](const CDamage& value) {
        return json{{"damage", value.damage}, {"damageType", sortedJsonArray(value.damageType)}};
    });
    appendComponentIfPresent<CEditorPlacement>(ecs, entity, components, "CEditorPlacement", [](const CEditorPlacement& value) {
        return json{{"definition", value.definition}, {"x", value.x}, {"y", value.y}};
    });
    appendComponentIfPresent<CAttackHitbox>(ecs, entity, components, "CAttackHitbox", [](const CAttackHitbox& value) {
        return json{{"owner", value.owner}, {"hitEntities", sortedJsonArray(value.hitEntities)}};
    });
    appendComponentIfPresent<CAttackState>(ecs, entity, components, "CAttackState", [](const CAttackState& value) {
        return json{{"direction", vec2Json(value.direction)}, {"attackHitFrame", value.attackHitFrame},
                    {"animationFrameCount", value.animationFrameCount}, {"animationFrameDuration", value.animationFrameDuration},
                    {"elapsedFrames", value.elapsedFrames}, {"hasFired", value.hasFired},
                    {"hasAnimationOverride", value.hasAnimationOverride}, {"hadAnimation", value.hadAnimation},
                    {"previousSprite", spriteJson(value.previousSprite)}, {"previousAnimation", animationJson(value.previousAnimation)}};
    });
    appendComponentIfPresent<CText>(ecs, entity, components, "CText", [](const CText& value) {
        return json{{"size", vec2Json(value.size)}, {"text", value.text}, {"font_name", value.font_name}};
    });
    appendComponentIfPresent<CPossessable>(ecs, entity, components, "CPossessable", [](const CPossessable& value) {
        return json{{"level", value.level}, {"duration", value.duration}, {"timeLeft", value.timeLeft},
                    {"lifeForce", value.lifeForce}, {"state", possessStateName(value.state)}};
    });
    appendComponentIfPresent<CAIAgent>(ecs, entity, components, "CAIAgent", [](const CAIAgent& value) {
        return json{{"sightRange", value.sightRange}, {"canSeePlayer", value.canSeePlayer},
                    {"lastKnownPlayerPos", vec2Json(value.lastKnownPlayerPos)}, {"memoryTimer", value.memoryTimer},
                    {"memoryDuration", value.memoryDuration}, {"spawnPos", vec2Json(value.spawnPos)},
                    {"patrolRadius", value.patrolRadius}, {"patrolTarget", vec2Json(value.patrolTarget)},
                    {"hasPatrolTarget", value.hasPatrolTarget}, {"patrolWaitTimer", value.patrolWaitTimer},
                    {"patrolWaitDuration", value.patrolWaitDuration}, {"state", aiStateName(value.state)}};
    });
    appendComponentIfPresent<CItem>(ecs, entity, components, "CItem", [](const CItem& value) {
        return json{{"itemID", value.itemID}, {"hasPickupModeOverride", value.hasPickupModeOverride},
                    {"pickupModeOverride", pickupModeName(value.pickupModeOverride)}};
    });
    appendComponentIfPresent<CInventory>(ecs, entity, components, "CInventory", [](const CInventory& value) {
        json items = json::array();
        for (const Item& item : value.items) {
            items.push_back(itemJson(item));
        }
        return json{{"activeItem", itemJson(value.activeItem)}, {"items", items}};
    });
    appendComponentIfPresent<CKnockback>(ecs, entity, components, "CKnockback", [](const CKnockback& value) {
        return json{{"duration", value.duration}, {"magnitude", value.magnitude},
                    {"direction", vec2Json(value.direction)}, {"timeElapsed", value.timeElapsed}};
    });
    appendComponentIfPresent<CWeapon>(ecs, entity, components, "CWeapon", [](const CWeapon& value) {
        return json{{"damage", value.damage}, {"speed", value.speed}, {"delay", value.delay}, {"range", value.range},
                    {"attackAnimation", value.attackAnimation}, {"attackAnimationRow", value.attackAnimationRow},
                    {"attackHitFrame", value.attackHitFrame}, {"hitboxSprite", value.hitboxSprite},
                    {"hitboxAnimation", value.hitboxAnimation}, {"hitboxActiveFrames", value.hitboxActiveFrames},
                    {"weaponType", weaponTypeName(value.weaponType)}, {"targetMask", collisionMaskJson(value.targetMask)}};
    });
    appendComponentIfPresent<CEvent>(ecs, entity, components, "CEvent", [](const CEvent& value) {
        return json{{"questID", value.questID}, {"event", {
            {"type", eventTypeName(value.event.type)}, {"itemName", value.event.itemName},
            {"eventPosition", vec2Json(value.event.eventPosition)}
        }}};
    });
    appendComponentIfPresent<CChild>(ecs, entity, components, "CChild", [](const CChild& value) {
        json children = json::array();
        for (const ChildLink& child : value.children) {
            children.push_back({{"child", child.child}, {"removeOnDeath", child.removeOnDeath}});
        }
        return json{{"children", children}};
    });
    appendComponentIfPresent<CStatic>(ecs, entity, components, "CStatic", [](const CStatic&) {
        return json::object();
    });
    appendComponentIfPresent<CChunk>(ecs, entity, components, "CChunk", [](const CChunk&) {
        return json::object();
    });

    return result;
}

std::vector<EntityID> findInspectableEntitiesAt(const ECS& ecs, Vec2 worldPoint)
{
    std::vector<EntityID> entities;
    for (const auto [entity, transform, collider] : ecs.constView<CTransform, CCollider>()) {
        if (!ecs.isAlive(entity) || ecs.hasComponent<CStatic>(entity) || ecs.hasComponent<CChunk>(entity)) {
            continue;
        }
        if (pointInCollider(worldPoint, transform, collider)) {
            entities.push_back(entity);
        }
    }
    std::sort(entities.begin(), entities.end());
    return entities;
}

} // namespace DebugEntityInspector
