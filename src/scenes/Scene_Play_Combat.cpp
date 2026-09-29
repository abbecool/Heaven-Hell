#include "scenes/Scene_Play.hpp"

#include "external/json.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

using json = nlohmann::json;

void Scene_Play::startAttack(EntityID attackerID, Vec2 direction,
                             CWeapon& weapon)
{
    if (direction.isNull())
    {
        direction = Vec2{1, 0};
    }

    CAttackState& attackState =
        m_ECS.addComponent<CAttackState>(attackerID, direction);

    if (!weapon.attackAnimation.empty() &&
        m_ECS.hasComponent<CSprite>(attackerID))
    {
        attackState.hasAnimationOverride = true;
        attackState.previousSprite = m_ECS.getComponent<CSprite>(attackerID);
        attackState.hadAnimation = m_ECS.hasComponent<CAnimation>(attackerID);
        if (attackState.hadAnimation)
        {
            attackState.previousAnimation =
                m_ECS.getComponent<CAnimation>(attackerID);
        }
        setAnimation(attackerID, weapon.attackAnimation, true);
    }

    if (weapon.attackAnimationRow >= 0 &&
        m_ECS.hasComponent<CAnimation>(attackerID))
    {
        CAnimation& animation = m_ECS.getComponent<CAnimation>(attackerID);
        animation.currentFrame = 0;
        animation.currentCol = 0;
        animation.currentRow = weapon.attackAnimationRow;
    }

    if (m_ECS.hasComponent<CAnimation>(attackerID))
    {
        CAnimation& animation = m_ECS.getComponent<CAnimation>(attackerID);
        animation.currentFrame = 0;
        animation.currentCol = 0;
        attackState.animationFrameCount =
            static_cast<int>(animation.frameCount);
        attackState.animationFrameDuration =
            static_cast<int>(animation.frameDuration);
    }

    attackState.attackHitFrame =
        std::clamp(weapon.attackHitFrame, 0,
                   std::max(0, attackState.animationFrameCount - 1));
}

void Scene_Play::finishAttack(EntityID attackerID, CAttackState& attackState,
                              const CWeapon* weapon)
{
    if (attackState.hasAnimationOverride &&
        m_ECS.hasComponent<CSprite>(attackerID))
    {
        m_ECS.getComponent<CSprite>(attackerID) = attackState.previousSprite;
        if (attackState.hadAnimation)
        {
            m_ECS.getComponent<CAnimation>(attackerID) =
                attackState.previousAnimation;
        }
        else if (m_ECS.hasComponent<CAnimation>(attackerID))
        {
            m_ECS.removeComponent<CAnimation>(attackerID);
        }
    }
    else if (weapon != nullptr && weapon->attackAnimationRow >= 0 &&
             m_ECS.hasComponent<CAnimation>(attackerID))
    {
        CAnimation& animation = m_ECS.getComponent<CAnimation>(attackerID);
        animation.currentRow = static_cast<int>(PlayerState::STAND);
        animation.currentFrame = 0;
        animation.currentCol = 0;
        if (attackerID == m_player && m_ECS.hasComponent<CState>(attackerID))
        {
            CState& state = m_ECS.getComponent<CState>(attackerID);
            state.state = PlayerState::STAND;
            state.preState = PlayerState::STAND;
            state.changeAnimate = true;
        }
    }

    m_ECS.removeComponent<CAttackState>(attackerID);
}

void Scene_Play::sAttack()
{
    ComponentPool<CWeapon>& weaponPool =
        m_ECS.getOrCreateComponentPool<CWeapon>();

    for (auto [id, inputs, inventory, transform] :
         m_ECS.View<CInput, CInventory, CTransform>())
    {
        Item& activeItem = inventory.activeItem;

        if (m_ECS.hasComponent<CAttackState>(id))
        {
            CAttackState& attack = m_ECS.getComponent<CAttackState>(id);
            if (!m_ECS.hasComponent<CWeapon>(id))
            {
                finishAttack(id, attack, nullptr);
                inputs.use = false;
                continue;
            }

            CWeapon& weapon = weaponPool.getComponent(id);
            if (id != m_player)
            {
                inputs.direction = {0, 0};
            }
            inputs.use = false;

            int attackGameFrame = ++attack.elapsedFrames;
            if (m_ECS.hasComponent<CAnimation>(id))
            {
                attackGameFrame =
                    static_cast<int>(
                        m_ECS.getComponent<CAnimation>(id).currentFrame) +
                    1;
            }

            if (!attack.hasFired && !inputs.useHeld)
            {
                finishAttack(id, attack, &weapon);
                continue;
            }

            if (!attack.hasFired && attackGameFrame >= attack.hitGameFrame())
            {
                Vec2 attackDirection = attack.direction;
                if (id == m_player &&
                    weapon.weaponType == WeaponType::Projectile)
                {
                    attackDirection = (getMousePosition() - transform.pos +
                                       getCameraPosition())
                                          .norm();
                    if (attackDirection.isNull())
                    {
                        attackDirection = attack.direction;
                    }
                    attack.direction = attackDirection;
                }
                switch (weapon.weaponType)
                {
                case WeaponType::Melee:
                    spawnHitbox(id, attackDirection, weapon);
                    break;
                case WeaponType::Projectile:
                    spawnProjectile(id, attackDirection, weapon);
                    break;
                case WeaponType::AoE:
                    spawnHitbox(id, attackDirection, weapon);
                    break;
                }
                attack.hasFired = true;
            }

            if (attackGameFrame >= attack.finishGameFrame())
            {
                finishAttack(id, attack, &weapon);
            }
            continue;
        }

        if (m_ECS.hasComponent<CWeapon>(id))
        {
            weaponPool.getComponent(id).delay--;
        }

        if (!inputs.use)
        {
            continue;
        }

        switch (activeItem.type)
        {
        case ItemType::Consumable:
            useActiveConsumable(id);
            inputs.use = false;
            continue;
        case ItemType::WeaponMelee:
        case ItemType::WeaponRanged:
        case ItemType::WeaponAoE:
            break;
        case ItemType::None:
        case ItemType::Weapon:
        case ItemType::Quest:
        case ItemType::Currency:
            inputs.use = false;
            continue;
        }

        if (!m_ECS.hasComponent<CWeapon>(id))
        {
            updateActiveItem(id, activeItem.index);
        }
        if (!m_ECS.hasComponent<CWeapon>(id))
        {
            inputs.use = false;
            continue;
        }

        CWeapon& weapon = weaponPool.getComponent(id);
        if (weapon.delay >= 0)
        {
            continue;
        }
        else
        {
            weapon.delay = weapon.speed;
        }
        Vec2 position = transform.pos;
        Vec2 direction = {0, 0};
        if (m_ECS.hasComponent<CVelocity>(id))
        {
            direction = m_ECS.getComponent<CVelocity>(id).vel;
        }
        if (id == m_player)
        {
            direction =
                (getMousePosition() - position + getCameraPosition()).norm();
        }
        else if (!inputs.direction.isNull())
        {
            direction = inputs.direction.norm();
        }
        if (direction.isNull())
        {
            direction = Vec2{1, 0};
        }

        startAttack(id, direction, weapon);
        inputs.use = false;
    }
}

void Scene_Play::updateSwimmingState(
    const std::unordered_set<EntityID>& activeWaterEntities)
{
    for (auto [entityID, swimming] : m_ECS.View<CSwimming>())
    {
        if (activeWaterEntities.find(entityID) != activeWaterEntities.end())
        {
            continue;
        }

        if (swimming.childEntity != 0 && m_ECS.isAlive(swimming.childEntity))
        {
            m_ECS.queueRemoveEntity(swimming.childEntity);
        }

        m_ECS.queueRemoveComponent<CSwimming>(entityID);
    }
}

EntityID Scene_Play::spawnSwimming(EntityID entityID)
{
    if (!m_ECS.hasComponent<CSwimming>(entityID))
    {
        m_ECS.addComponent<CSwimming>(entityID);
    }

    auto& swimming = m_ECS.getComponent<CSwimming>(entityID);
    if (swimming.childEntity != 0 && m_ECS.isAlive(swimming.childEntity))
    {
        return swimming.childEntity;
    }

    const EntityID swimmingID = m_ECS.addEntity();
    swimming.childEntity = swimmingID;

    m_ECS.attachChild(entityID, swimmingID, Vec2{0, 0});
    m_ECS.addComponent<CTransform>(swimmingID);
    addVisual(swimmingID, "swimming", RenderLayer::WorldProp, true);
    return swimmingID;
}

EntityID Scene_Play::spawnShadow(EntityID parentID, const CShadow& shadowConfig)
{
    const CTransform& parentTransform =
        m_ECS.getComponent<CTransform>(parentID);
    const CSprite& parentSprite = m_ECS.getComponent<CSprite>(parentID);
    const SpriteDefinition& shadowSprite = getSprite("shadow");

    const Vec2 parentSize = parentSprite.size() * parentTransform.scale;
    const Vec2 shadowSize = shadowSprite.frameSize();
    const float shadowScale = parentSize.x / shadowSize.x * shadowConfig.scale;
    const Vec2 scaledShadowSize = shadowSize * shadowScale;
    const Vec2 relativePos{shadowConfig.offset.x,
                           parentSize.y / 2.0f - scaledShadowSize.y / 2.0f +
                               shadowConfig.offset.y};

    auto shadowID = m_ECS.addEntity();
    m_ECS.addComponent<CTransform>(shadowID);
    m_ECS.getComponent<CTransform>(shadowID).scale = {shadowScale, shadowScale};
    addSprite(shadowID, "shadow", parentSprite.layer - 1);
    m_ECS.attachChild(parentID, shadowID, relativePos);
    return shadowID;
}

EntityID Scene_Play::spawnProjectile(EntityID attackerID, Vec2 vel,
                                     const CWeapon& weapon)
{
    auto id = m_ECS.addEntity();
    const int layer = RenderLayer::Projectile;
    const float speed = 200.0f;
    const int flightLifetime = 60;
    const float createOffset = 12.0f;
    const Vec2 startPos = m_ECS.getComponent<CTransform>(attackerID).pos;

    Vec2 direction = vel.norm();
    if (direction.isNull())
    {
        direction = Vec2{1, 0};
    }

    addVisual(id, "fireball", layer, true);
    m_ECS.addComponent<CTransform>(id, startPos + direction * createOffset,
                                   direction.angle());
    m_ECS.addComponent<CProjectile>(id, attackerID, direction, speed,
                                    flightLifetime, createOffset,
                                    weapon.targetMask);
    m_ECS.addComponent<CProjectileState>(id, ProjectilePhase::Flying);
    m_ECS.addComponent<CDamage>(id, weapon.damage);
    m_ECS.getComponent<CDamage>(id).damageType = {"Fire", "Explosive"};
    m_ECS.addComponent<CVelocity>(id, direction.norm(speed));
    m_ECS.addComponent<CCollider>(id, Vec2{6, 6}, PROJECTILE_LAYER,
                                  weapon.targetMask | OBSTACLE_LAYER);
    m_ECS.addComponent<CAudio>(id, "fireball_shot");
    m_camera.startShake(2, 60);
    return id;
}

void Scene_Play::destroyProjectile(EntityID projectileID)
{
    if (!m_ECS.hasComponent<CProjectileState>(projectileID))
    {
        return;
    }

    auto& projectileState = m_ECS.getComponent<CProjectileState>(projectileID);
    if (projectileState.phase == ProjectilePhase::Destroying)
    {
        return;
    }

    projectileState.phase = ProjectilePhase::Destroying;
    if (m_ECS.hasComponent<CParent>(projectileID))
    {
        m_ECS.detachChild(projectileID);
    }
    if (m_ECS.hasComponent<CVelocity>(projectileID))
    {
        m_ECS.getComponent<CVelocity>(projectileID).vel = Vec2{0, 0};
    }
    if (m_ECS.hasComponent<CCollider>(projectileID))
    {
        m_ECS.queueRemoveComponent<CCollider>(projectileID);
    }

    setAnimation(projectileID, "fireball_explode", false);
    m_ECS.addComponent<CAudio>(projectileID, "fireball_destroy");
}

EntityID Scene_Play::spawnHitbox(EntityID attackerID, Vec2 direction,
                                 const CWeapon& weapon)
{
    auto id = m_ECS.addEntity();
    const int renderLayer = RenderLayer::AttackEffect;
    if (direction.isNull())
    {
        direction = Vec2{1, 0};
    }

    const Vec2 attackerPosition =
        m_ECS.getComponent<CTransform>(attackerID).pos;
    const Vec2 hitboxSize = Vec2{weapon.range, weapon.range};
    Vec2 hitboxPosition = attackerPosition;

    if (!weapon.hitboxAnimation.empty())
    {
        addVisual(id, weapon.hitboxAnimation, renderLayer, false);
    }
    else
    {
        addSprite(id, weapon.hitboxSprite, renderLayer);
        hitboxPosition += direction.norm((float)weapon.range / 2.0f);
    }

    m_ECS.addComponent<CTransform>(id, hitboxPosition);
    m_ECS.addComponent<CDamage>(id, weapon.damage);
    m_ECS.addComponent<CAttackHitbox>(id, attackerID);
    m_ECS.addComponent<CCollider>(id, hitboxSize, DAMAGE_LAYER,
                                  weapon.targetMask, Color{0, 0, 255, 255},
                                  true);

    if (weapon.hitboxAnimation.empty())
    {
        m_ECS.addComponent<CLifespan>(id,
                                      std::max(1, weapon.hitboxActiveFrames));
    }
    else
    {
        m_ECS.addComponent<CActiveHitboxLifetime>(
            id, std::max(1, weapon.hitboxActiveFrames));
    }
    return id;
}

void Scene_Play::changePlayerState(EntityID entity, PlayerState s)
{
    CState& entityState = m_ECS.getComponent<CState>(entity);
    auto& prev = entityState.preState;
    if (prev != s)
    {
        prev = entityState.state;
        entityState.state = s;
        entityState.changeAnimate = true;
    }
    else
    {
        entityState.changeAnimate = false;
    }
    if (s != PlayerState::STAND)
    {
        entityState.facing = s;
    }
}

bool Scene_Play::rayIntersectsAABB(Vec2 origin, Vec2 dir, float maxDist,
                                   Vec2 boxMin, Vec2 boxMax)
{
    float tMin = 0.0f;
    float tMax = maxDist;

    if (std::abs(dir.x) < 1e-6f)
    {
        if (origin.x < boxMin.x || origin.x > boxMax.x)
            return false;
    }
    else
    {
        float t1 = (boxMin.x - origin.x) / dir.x;
        float t2 = (boxMax.x - origin.x) / dir.x;
        if (t1 > t2)
            std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax)
            return false;
    }

    if (std::abs(dir.y) < 1e-6f)
    {
        if (origin.y < boxMin.y || origin.y > boxMax.y)
            return false;
    }
    else
    {
        float t1 = (boxMin.y - origin.y) / dir.y;
        float t2 = (boxMax.y - origin.y) / dir.y;
        if (t1 > t2)
            std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax)
            return false;
    }

    return tMin <= tMax;
}

bool Scene_Play::hasLineOfSight(Vec2 origin, Vec2 target)
{
    Vec2 delta = target - origin;
    float dist = delta.length();
    if (dist <= 0.0001f)
    {
        return true;
    }
    Vec2 dir = delta / dist;

    for (auto [obstacle, collider, transform, _] :
         m_ECS.constView<CCollider, CTransform, CStatic>())
    {
        for (const auto& shape : collider.shapes)
        {
            if (shape.isTrigger || (shape.layer & OBSTACLE_LAYER) == 0)
            {
                continue;
            }

            Vec2 pos = transform.pos + shape.offset;
            Vec2 boxMin = pos - shape.halfSize;
            Vec2 boxMax = pos + shape.halfSize;

            if (rayIntersectsAABB(origin, dir, dist, boxMin, boxMax))
            {
                return false;
            }
        }
    }
    return true;
}

void Scene_Play::tickPatrol(CAIAgent& agent, Vec2 pos, CInput& input)
{
    if (agent.patrolWaitTimer > 0)
    {
        agent.patrolWaitTimer--;
        input.direction = {0, 0};
        return;
    }

    if (!agent.hasPatrolTarget)
    {
        Vec2 offset;
        do
        {
            float rx = ((rand() % 200) - 100) / 100.0f;
            float ry = ((rand() % 200) - 100) / 100.0f;
            offset = Vec2{rx, ry} * agent.patrolRadius;
        } while (offset.length() > agent.patrolRadius);

        agent.patrolTarget = agent.spawnPos + offset;
        agent.hasPatrolTarget = true;
    }

    Vec2 toTarget = agent.patrolTarget - pos;
    float dist = toTarget.length();

    if (dist < 8.0f)
    {
        agent.hasPatrolTarget = false;
        agent.patrolWaitTimer = agent.patrolWaitDuration;
        input.direction = {0, 0};
    }
    else
    {
        input.direction = toTarget.norm();
    }
}

bool Scene_Play::tryPossess(EntityID player, EntityID mob)
{
    if (player != m_player || !m_ECS.isAlive(player) || !m_ECS.isAlive(mob) ||
        !m_ECS.hasComponent<CInput>(m_player) ||
        !m_ECS.hasComponent<CPossessable>(mob) ||
        !m_ECS.hasComponent<CHealth>(mob) ||
        !m_ECS.hasComponent<CHealth>(m_player))
    {
        return false;
    }

    if (m_ECS.hasComponent<CLifespan>(mob) &&
        m_ECS.getComponent<CLifespan>(mob).lifespan <= 0)
    {
        return false;
    }

    CInput& input = m_ECS.getComponent<CInput>(m_player);
    CPossessable& mobPosses = m_ECS.getComponent<CPossessable>(mob);

    if (!input.possesHeld)
    {
        const std::string text =
            (mobPosses.state == PossessState::Drain) ? "Drain" : "Possess";
        SpawnDialog(text, 12, "Minecraft", mob);
        mobPosses.timeLeft = mobPosses.duration;
        if ((mobPosses.timeLeft != mobPosses.duration) &&
            (mobPosses.state == PossessState::Drain))
        {
            m_ECS.addComponent<CInput>(mob);
        }
        return false;
    }

    CHealth& mobHealth = m_ECS.getComponent<CHealth>(mob);
    CHealth& playerHealth = m_ECS.getComponent<CHealth>(m_player);

    switch (mobPosses.state)
    {
    case PossessState::Drain:
    {
        if (input.posses)
        {
            input.posses = false;
            EntityID effect = m_ECS.addEntity();
            addVisual(effect, "beingPossessed", RenderLayer::GroundItem, false);
            m_ECS.addComponent<CTransform>(effect);
            m_ECS.attachChild(mob, effect, Vec2{0, -4});
            m_ECS.removeComponent<CInput>(mob);
            mobPosses.lifeForce = mobHealth.HP;
        }

        mobPosses.timeLeft--;
        if (mobPosses.duration > 0 &&
            mobPosses.timeLeft != mobPosses.duration &&
            (mobPosses.timeLeft * mobPosses.lifeForce) % mobPosses.duration ==
                0)
        {
            playerHealth.HP--;
            mobHealth.HP--;
        }

        if (mobPosses.timeLeft > 0)
        {
            return false;
        }

        m_ECS.addComponent<CLifespan>(mob, 180);
        playerHealth.HP += static_cast<int>(1.5 * mobPosses.lifeForce);
        mobPosses.state = PossessState::Possess;
        Emit(Event{EventType::EntityDrained,
                   m_ECS.getComponent<CName>(mob).name});

        return false;
    }
    case PossessState::Possess:
    {
        if (!input.posses)
        {
            return false;
        }
        m_ECS.removeComponent<CPossessable>(mob);
        const std::string possessedName = m_ECS.getComponent<CName>(mob).name;
        EntityID oldID = m_player;
        EntityID newID = mob;
        bool hasPossessedActiveItem = false;
        Item possessedActiveItem;

        if (m_ECS.hasComponent<CInventory>(mob))
        {
            possessedActiveItem =
                m_ECS.getComponent<CInventory>(mob).activeItem;
            hasPossessedActiveItem = possessedActiveItem.id != -1;
            if (possessedActiveItem.hasWeaponConfig)
            {
                possessedActiveItem.weaponConfig["mask"] =
                    nlohmann::json::array({"ENEMY_LAYER"});
            }
        }

        m_ECS.copyComponent<CCollider>(newID, oldID);
        m_ECS.copyComponent<CInventory>(newID, oldID);
        m_ECS.copyComponent<CHealth>(newID, oldID);
        if (m_ECS.hasComponent<CCurrency>(m_player))
        {
            m_ECS.copyComponent<CCurrency>(newID, oldID);
        }
        m_ECS.copyComponent<CState>(newID, oldID);

        if (m_ECS.hasComponent<CWeapon>(m_player))
        {
            m_ECS.copyComponent<CWeapon>(newID, oldID);
        }
        else if (m_ECS.hasComponent<CWeapon>(mob))
        {
            m_ECS.removeComponent<CWeapon>(newID);
        }

        if (!m_ECS.hasComponent<CInput>(mob))
        {
            m_ECS.addComponent<CInput>(mob);
        }
        if (m_ECS.hasComponent<CAIAgent>(mob))
        {
            m_ECS.removeComponent<CAIAgent>(mob);
        }
        if (m_ECS.hasComponent<CLifespan>(mob))
        {
            m_ECS.removeComponent<CLifespan>(mob);
        }

        if (m_playerPlacement != static_cast<size_t>(-1))
            m_removedPlacements.insert(m_playerPlacement);
        const auto placed = m_placedEntities.find(newID);
        m_playerPlacement = placed != m_placedEntities.end()
                                ? placed->second : static_cast<size_t>(-1);
        if (placed != m_placedEntities.end())
            m_placedEntities.erase(placed);
        m_playerDefinition = possessedName;
        changePlayerID(newID);
        if (hasPossessedActiveItem)
        {
            addItemToInventory(mob, possessedActiveItem);
        }

        m_ECS.queueRemoveEntity(oldID);
        Emit(Event{EventType::EntityPossessed, possessedName});
        return true;
    }
    default:
        return false;
    }
}

bool Scene_Play::addItemToInventory(EntityID character, const Item& item)
{
    if (!m_ECS.hasComponent<CInventory>(character))
    {
        return false;
    }

    auto& inventory = m_ECS.getComponent<CInventory>(character);
    auto& activeItem = inventory.activeItem;
    for (int i = 0; i < inventory.size(); ++i)
    {
        auto& slot = inventory.items[i];
        if (slot.id != -1)
        {
            continue;
        }

        int index = slot.index;
        slot = item;
        slot.index = index;
        if (index == activeItem.index)
        {
            updateActiveItem(index);
        }
        return true;
    }

    if (!m_ECS.hasComponent<CTransform>(character))
    {
        return false;
    }

    const int activeIndex = activeItem.index;
    if (activeIndex < 0 || activeIndex >= inventory.size())
    {
        return false;
    }

    Item droppedItem = inventory.items[activeIndex];
    if (droppedItem.id == -1)
    {
        return false;
    }

    EntityID droppedID =
        DropItem(droppedItem, m_ECS.getComponent<CTransform>(character).pos);
    if (droppedID == static_cast<EntityID>(-1))
    {
        return false;
    }

    inventory.items[activeIndex] = item;
    inventory.items[activeIndex].index = activeIndex;
    updateActiveItem(activeIndex);
    return true;
}
