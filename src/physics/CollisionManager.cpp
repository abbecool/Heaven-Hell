#include "physics/CollisionManager.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

#include "ecs/ScriptableEntity.hpp"
#include "scenes/Scene_Play.hpp"

namespace {

float dot(const Vec2& a, const Vec2& b)
{
    return a.x * b.x + a.y * b.y;
}

float inverseMass(Entity& entity)
{
    if (!entity.hasComponent<CPhysicsBody>()) {
        return 0.0f;
    }
    return 1.0f / entity.getComponent<CPhysicsBody>().mass;
}

void removeInwardVelocity(Entity& entity, const Vec2& outwardNormal)
{
    if (outwardNormal.isNull() || !entity.hasComponent<CVelocity>()) {
        return;
    }

    CVelocity& velocity = entity.getComponent<CVelocity>();
    const float normalVelocity = dot(velocity.vel, outwardNormal);
    if (normalVelocity < 0.0f) {
        velocity.vel -= outwardNormal * normalVelocity;
    }
}

void resolveBodyCollision(Entity entityA, Entity entityB, const Vec2& overlap)
{
    if (overlap.isNull()) {
        return;
    }

    const float inverseMassA = inverseMass(entityA);
    const float inverseMassB = inverseMass(entityB);
    const float inverseMassTotal = inverseMassA + inverseMassB;
    if (inverseMassTotal == 0.0f) {
        return;
    }

    CTransform& transformA = entityA.getComponent<CTransform>();
    CTransform& transformB = entityB.getComponent<CTransform>();
    const Vec2 outwardNormalA = overlap.norm();

    transformA.pos += overlap * (inverseMassA / inverseMassTotal);
    transformB.pos -= overlap * (inverseMassB / inverseMassTotal);

    removeInwardVelocity(entityA, outwardNormalA);
    removeInwardVelocity(entityB, outwardNormalA * -1.0f);
}

void handleBodyCollision(Entity entityA, Entity entityB, Vec2 overlap)
{
    resolveBodyCollision(entityA, entityB, overlap);
}

bool isFlyingProjectile(Entity projectile)
{
    return projectile.hasComponent<CProjectileState>() &&
        projectile.getComponent<CProjectileState>().phase == ProjectilePhase::Flying;
}

void flashDamage(Entity target)
{
    constexpr int DamageFlashFrames = 8;
    if (target.hasComponent<CDamageFlash>()) {
        target.getComponent<CDamageFlash>().reset();
        return;
    }
    target.addComponent<CDamageFlash>(DamageFlashFrames);
}

void applyDamage(Entity target, int damage)
{
    target.getComponent<CHealth>().HP -= damage;
    flashDamage(target);
}

void handleProjectileHit(Scene_Play* scene, Entity target, Entity projectile)
{
    if (!isFlyingProjectile(projectile) ||
        !projectile.hasComponent<CProjectile>() ||
        !projectile.hasComponent<CDamage>() ||
        !target.hasComponent<CHealth>()) {
        return;
    }

    if (target.getID() == projectile.getComponent<CProjectile>().owner) {
        return;
    }

    applyDamage(target, projectile.getComponent<CDamage>().damage);
    scene->destroyProjectile(projectile.getID());
}

uint64_t hashCombine(uint64_t seed, uint64_t value)
{
    return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
}

} // namespace

uint8_t CollisionManager::layerIndex(CollisionMask layer)
{
    const auto value = layer.to_ulong();
    if (value == 0) {
        return 0;
    }
    return static_cast<uint8_t>(std::log2(static_cast<double>(value)));
}

uint64_t CollisionManager::pairKey(size_t first, size_t second)
{
    if (first > second) {
        std::swap(first, second);
    }
    return (static_cast<uint64_t>(first) << 32) | static_cast<uint64_t>(second);
}

uint64_t CollisionManager::triggerPairKey(const ColliderProxy& first, const ColliderProxy& second)
{
    EntityID entityA = first.entity;
    EntityID entityB = second.entity;
    uint8_t layerA = layerIndex(first.layer);
    uint8_t layerB = layerIndex(second.layer);

    if (entityA > entityB || (entityA == entityB && layerA > layerB)) {
        std::swap(entityA, entityB);
        std::swap(layerA, layerB);
    }

    uint64_t key = 1469598103934665603ULL;
    key = hashCombine(key, entityA);
    key = hashCombine(key, entityB);
    key = hashCombine(key, layerA);
    key = hashCombine(key, layerB);
    return key;
}

bool CollisionManager::layersMatch(const ColliderProxy& first, const ColliderProxy& second)
{
    const bool firstTargetsSecond = (second.layer & first.targetMask) == second.layer;
    const bool secondTargetsFirst = (first.layer & second.targetMask) == first.layer;
    return firstTargetsSecond || secondTargetsFirst;
}

bool CollisionManager::aabbIntersects(const ColliderProxy& first, const ColliderProxy& second)
{
    const Vec2 firstMin = first.center - first.halfSize;
    const Vec2 secondMin = second.center - second.halfSize;

    const bool xOverlap = (firstMin.x + first.size.x > secondMin.x) &&
        (secondMin.x + second.size.x > firstMin.x);
    const bool yOverlap = (firstMin.y + first.size.y > secondMin.y) &&
        (secondMin.y + second.size.y > firstMin.y);

    return xOverlap && yOverlap;
}

bool CollisionManager::hasValidWorldBounds() const
{
    return m_worldSize.x > 0.0f && m_worldSize.y > 0.0f;
}

void CollisionManager::insertColliderProxy(
    EntityID entityID,
    size_t shapeIndex,
    const ColliderShape& shape,
    const CTransform& transform,
    bool isStatic,
    std::vector<ColliderProxy>& proxies,
    Quadtree& tree
)
{
    if (shape.size.x <= 0.0f || shape.size.y <= 0.0f || shape.layer == EMPTY_MASK) {
        return;
    }

    ColliderProxy proxy;
    proxy.entity = entityID;
    proxy.shapeIndex = shapeIndex;
    proxy.center = transform.pos + shape.offset;
    proxy.prevCenter = transform.prevPos + shape.offset;
    proxy.size = shape.size;
    proxy.halfSize = shape.halfSize;
    proxy.layer = shape.layer;
    proxy.targetMask = shape.targetMask;
    proxy.isTrigger = shape.isTrigger;
    proxy.isStatic = isStatic;

    const size_t proxyIndex = proxies.size();
    proxies.push_back(proxy);
    tree.insert(proxyIndex, proxy.center, proxy.size);
}

Vec2 CollisionManager::calculateDelta(Vec2 aPos, Vec2 aSize, Vec2 bPos, Vec2 bSize) const
{
    return ((aPos + aSize) - (bPos + bSize)).abs_elem();
}

Vec2 CollisionManager::calculateHorizontalMovement(
    const Vec2& aPos,
    const Vec2& aSize,
    const Vec2& bPos,
    const Vec2& bSize,
    const Vec2& overlap,
    const Vec2& prevOverlap
) const
{
    Vec2 move = {0, 0};
    if (prevOverlap.y <= 0) {
        return move;
    }

    if ((aPos.x + aSize.x) > (bPos.x + bSize.x)) {
        move += Vec2{overlap.x, 0.0f};
    }
    if ((aPos.x + aSize.x) < (bPos.x + bSize.x)) {
        move -= Vec2{overlap.x, 0.0f};
    }
    return move;
}

Vec2 CollisionManager::calculateVerticalMovement(
    const Vec2& aPos,
    const Vec2& aSize,
    const Vec2& bPos,
    const Vec2& bSize,
    const Vec2& overlap,
    const Vec2& prevOverlap
) const
{
    Vec2 move = {0, 0};
    if (prevOverlap.x <= 0) {
        return move;
    }

    if ((aPos.y + aSize.y / 2) > (bPos.y + bSize.y / 2)) {
        move += Vec2{0.0f, overlap.y};
    }
    if ((aPos.y + aSize.y / 2) < (bPos.y + bSize.y / 2)) {
        move -= Vec2{0.0f, overlap.y};
    }
    return move;
}

Vec2 CollisionManager::collisionOverlap(const ColliderProxy& first, const ColliderProxy& second) const
{
    const Vec2 firstTopLeft = first.center - first.halfSize;
    const Vec2 secondTopLeft = second.center - second.halfSize;
    const Vec2 firstPrevTopLeft = first.prevCenter - first.halfSize;
    const Vec2 secondPrevTopLeft = second.prevCenter - second.halfSize;

    const Vec2 delta = calculateDelta(firstTopLeft, first.halfSize, secondTopLeft, second.halfSize);
    const Vec2 prevDelta = calculateDelta(firstPrevTopLeft, first.halfSize, secondPrevTopLeft, second.halfSize);
    const Vec2 overlap = first.halfSize + second.halfSize - delta;
    const Vec2 prevOverlap = first.halfSize + second.halfSize - prevDelta;

    const Vec2 horizontalMove = calculateHorizontalMovement(
        firstTopLeft,
        first.halfSize,
        secondTopLeft,
        second.halfSize,
        overlap,
        prevOverlap
    );
    const Vec2 verticalMove = calculateVerticalMovement(
        firstTopLeft,
        first.halfSize,
        secondTopLeft,
        second.halfSize,
        overlap,
        prevOverlap
    );

    return horizontalMove + verticalMove;
}

void CollisionManager::registerSolidHandler(CollisionMask layerA, CollisionMask layerB, Handler handler)
{
    uint8_t indexA = layerIndex(layerA);
    uint8_t indexB = layerIndex(layerB);

    if (indexA > indexB) {
        std::swap(indexA, indexB);
    }
    m_solidHandlers[indexA][indexB] = handler;
}

void CollisionManager::registerTriggerHandler(CollisionMask layerA, CollisionMask layerB, Handler handler)
{
    uint8_t indexA = layerIndex(layerA);
    uint8_t indexB = layerIndex(layerB);

    if (indexA > indexB) {
        std::swap(indexA, indexB);
    }
    m_triggerHandlers[indexA][indexB] = handler;
}

void CollisionManager::dispatch(
    CollisionMatrix& matrix,
    EntityID entityIDA,
    CollisionMask layerA,
    EntityID entityIDB,
    CollisionMask layerB,
    Vec2 overlap
)
{
    uint8_t indexA = layerIndex(layerA);
    uint8_t indexB = layerIndex(layerB);

    if (indexA > indexB) {
        std::swap(entityIDA, entityIDB);
        std::swap(indexA, indexB);
        overlap *= -1.0f;
    }

    Entity entityA = {entityIDA, m_ECS};
    Entity entityB = {entityIDB, m_ECS};

    if (matrix[indexA][indexB]) {
        matrix[indexA][indexB](entityA, entityB, overlap);
    }
}

void CollisionManager::buildQuadtree()
{
    m_proxies = m_staticProxies;
    m_processedShapePairs.clear();
    m_processedTriggerPairs.clear();

    if (!hasValidWorldBounds()) {
        m_quadRoot = nullptr;
        m_proxies.clear();
        return;
    }

    if (m_staticQuadRoot) {
        m_quadRoot = m_staticQuadRoot->clone();
    }
    else {
        m_quadRoot = std::make_unique<Quadtree>(m_worldCenter, m_worldSize);
    }

    for (auto [entityID, collider, transform] :
        m_ECS->constView<CCollider, CTransform>(ecs::Exclude<CStatic>{})) {
        for (size_t shapeIndex = 0; shapeIndex < collider.shapes.size(); ++shapeIndex) {
            const ColliderShape& shape = collider.shapes[shapeIndex];
            insertColliderProxy(
                entityID,
                shapeIndex,
                shape,
                transform,
                false,
                m_proxies,
                *m_quadRoot
            );
        }
    }
}

void CollisionManager::processQuadtreeLeaf(const std::vector<size_t>& proxyIndices)
{
    for (size_t a = 0; a < proxyIndices.size(); ++a) {
        const size_t proxyIndexA = proxyIndices[a];
        const ColliderProxy& proxyA = m_proxies[proxyIndexA];

        for (size_t b = a + 1; b < proxyIndices.size(); ++b) {
            const size_t proxyIndexB = proxyIndices[b];
            const ColliderProxy& proxyB = m_proxies[proxyIndexB];

            if (proxyA.entity == proxyB.entity) {
                continue;
            }

            if (proxyA.isStatic && proxyB.isStatic) {
                continue;
            }

            const uint64_t shapePair = pairKey(proxyIndexA, proxyIndexB);
            if (!m_processedShapePairs.insert(shapePair).second) {
                continue;
            }

            if (!layersMatch(proxyA, proxyB) || !aabbIntersects(proxyA, proxyB)) {
                continue;
            }

            if (proxyA.isTrigger || proxyB.isTrigger) {
                const uint64_t triggerPair = triggerPairKey(proxyA, proxyB);
                if (m_processedTriggerPairs.insert(triggerPair).second) {
                    dispatch(
                        m_triggerHandlers,
                        proxyA.entity,
                        proxyA.layer,
                        proxyB.entity,
                        proxyB.layer,
                        {0, 0}
                    );
                }
                continue;
            }

            const Vec2 overlap = collisionOverlap(proxyA, proxyB);
            dispatch(
                m_solidHandlers,
                proxyA.entity,
                proxyA.layer,
                proxyB.entity,
                proxyB.layer,
                overlap
            );
        }
    }
}

CollisionManager::CollisionManager(ECS* ecs, Scene_Play* scene)
    : m_ECS(ecs), m_scene(scene)
{
    registerSolidHandler(PLAYER_LAYER, OBSTACLE_LAYER, handleBodyCollision);
    registerSolidHandler(PLAYER_LAYER, ENEMY_LAYER, handleBodyCollision);
    registerSolidHandler(PLAYER_LAYER, FRIENDLY_LAYER, handleBodyCollision);
    registerSolidHandler(FRIENDLY_LAYER, OBSTACLE_LAYER, handleBodyCollision);
    registerSolidHandler(FRIENDLY_LAYER, FRIENDLY_LAYER, handleBodyCollision);
    registerSolidHandler(FRIENDLY_LAYER, ENEMY_LAYER, handleBodyCollision);
    registerSolidHandler(
        ENEMY_LAYER,
        PROJECTILE_LAYER,
        [this](Entity enemy, Entity projectile, Vec2 overlap) {
            handleProjectileHit(m_scene, enemy, projectile);
        }
    );
    registerSolidHandler(
        PLAYER_LAYER,
        PROJECTILE_LAYER,
        [this](Entity player, Entity projectile, Vec2 overlap) {
            handleProjectileHit(m_scene, player, projectile);
        }
    );
    registerSolidHandler(
        FRIENDLY_LAYER,
        PROJECTILE_LAYER,
        [this](Entity friendly, Entity projectile, Vec2 overlap) {
            handleProjectileHit(m_scene, friendly, projectile);
        }
    );
    registerSolidHandler(ENEMY_LAYER, ENEMY_LAYER, handleBodyCollision);
    registerSolidHandler(ENEMY_LAYER, OBSTACLE_LAYER, handleBodyCollision);
    registerSolidHandler(
        PROJECTILE_LAYER,
        OBSTACLE_LAYER,
        [this](Entity projectile, Entity obstacle, Vec2 overlap) {
            if (!isFlyingProjectile(projectile)) {
                return;
            }
            m_scene->destroyProjectile(projectile.getID());
        }
    );

    registerTriggerHandler(
        PLAYER_LAYER,
        FRIENDLY_LAYER,
        [this](Entity a, Entity b, Vec2 overlap) { handlePlayerFriendly(a, b, overlap); }
    );
    registerTriggerHandler(
        PLAYER_LAYER,
        LOOT_LAYER,
        [this](Entity a, Entity b, Vec2 overlap) { handlePlayerLoot(a, b, overlap); }
    );
    registerTriggerHandler(
        PLAYER_LAYER,
        AREA_LAYER,
        [this](Entity a, Entity b, Vec2 overlap) { handlePlayerArea(a, b, overlap); }
    );
    registerTriggerHandler(
        PLAYER_LAYER,
        WATER_LAYER,
        [this](Entity a, Entity b, Vec2 overlap) { handleMobWater(a, b, overlap); }
    );
    registerTriggerHandler(
        DAMAGE_LAYER,
        ENEMY_LAYER,
        [this](Entity a, Entity b, Vec2 overlap) { handleDamageHitbox(a, b, overlap); }
    );
    registerTriggerHandler(
        DAMAGE_LAYER,
        PLAYER_LAYER,
        [this](Entity a, Entity b, Vec2 overlap) { handleDamageHitbox(a, b, overlap); }
    );
    registerTriggerHandler(
        DAMAGE_LAYER,
        FRIENDLY_LAYER,
        [this](Entity a, Entity b, Vec2 overlap) { handleDamageHitbox(a, b, overlap); }
    );
}

void CollisionManager::setWorldBounds(Vec2 center, Vec2 size)
{
    m_worldCenter = center;
    m_worldSize = size;
    if (!hasValidWorldBounds()) {
        m_quadRoot = nullptr;
        m_staticQuadRoot = nullptr;
        m_staticProxies.clear();
        return;
    }
    m_staticProxies.clear();
    m_staticQuadRoot = std::make_unique<Quadtree>(m_worldCenter, m_worldSize);
    m_quadRoot = m_staticQuadRoot->clone();
}

void CollisionManager::rebuildStaticQuadtree()
{
    m_staticProxies.clear();
    if (!hasValidWorldBounds()) {
        m_staticQuadRoot = nullptr;
        m_quadRoot = nullptr;
        return;
    }

    m_staticQuadRoot = std::make_unique<Quadtree>(m_worldCenter, m_worldSize);
    for (auto [entityID, collider, transform, staticMarker] : m_ECS->constView<CCollider, CTransform, CStatic>()) {
        for (size_t shapeIndex = 0; shapeIndex < collider.shapes.size(); ++shapeIndex) {
            const ColliderShape& shape = collider.shapes[shapeIndex];
            insertColliderProxy(
                entityID,
                shapeIndex,
                shape,
                transform,
                true,
                m_staticProxies,
                *m_staticQuadRoot
            );
        }
    }

    m_quadRoot = m_staticQuadRoot->clone();
    m_proxies = m_staticProxies;
}

void CollisionManager::doCollisions()
{
    buildQuadtree();
    if (!m_quadRoot) {
        return;
    }

    auto quadVector = m_quadRoot->createQuadtreeVector();

    for (const auto& quadleaf : quadVector) {
        processQuadtreeLeaf(quadleaf->getObjects());
    }
}

void CollisionManager::renderQuadtree(RenderBackend& renderer)
{
    if (!m_quadRoot) {
        return;
    }
    m_quadRoot->renderBoundary(renderer, {255, 0, 0, 255});
}

void CollisionManager::handleDamageHitbox(Entity entityA, Entity entityB, Vec2 overlap)
{
    Entity hitbox = entityA;
    Entity target = entityB;
    if (!hitbox.hasComponent<CAttackHitbox>()) {
        std::swap(hitbox, target);
    }

    if (!hitbox.hasComponent<CAttackHitbox>() ||
        !hitbox.hasComponent<CDamage>() ||
        !target.hasComponent<CHealth>()) {
        return;
    }

    CAttackHitbox& attackHitbox = hitbox.getComponent<CAttackHitbox>();
    const EntityID targetID = target.getID();
    if (targetID == attackHitbox.owner ||
        attackHitbox.hitEntities.find(targetID) != attackHitbox.hitEntities.end()) {
        return;
    }

    attackHitbox.hitEntities.insert(targetID);
    applyDamage(target, hitbox.getComponent<CDamage>().damage);
}

bool CollisionManager::talkToNPC(Entity player, Entity friendly)
{
    if (!player.hasComponent<CInput>()) {
        return false;
    }
    if (!player.getComponent<CInput>().interact) {
        return false;
    }

    std::string name = friendly.getComponent<CName>().name;
    std::string currentDialog = m_scene->getStoryManager().getDialog(name);

    bool hasDialog = false;
    if (friendly.hasComponent<CChild>()) {
        for (const auto& childLink : friendly.getComponent<CChild>().children) {
            if (m_ECS->isAlive(childLink.child) && m_ECS->hasComponent<CText>(childLink.child)) {
                hasDialog = true;
                break;
            }
        }
    }

    if (!hasDialog) {
        m_scene->SpawnDialog(currentDialog, 16, "Minecraft", friendly.getID());
    }
    m_scene->Emit(Event{EventType::DialogueFinished, name});
    return true;
}

bool CollisionManager::possesNPC(Entity player, Entity friendly)
{
    if (!player.hasComponent<CInput>()) {
        return false;
    }
    if (!player.getComponent<CInput>().posses) {
        return false;
    }
    if (!friendly.hasComponent<CPossesLevel>()) {
        return false;
    }

    friendly.removeComponent<CPossesLevel>();
    EntityID oldID = player.getID();
    EntityID newID = friendly.getID();

    m_ECS->copyComponent<CCollider>(newID, oldID);
    m_ECS->copyComponent<CInventory>(newID, oldID);
    m_ECS->copyComponent<CHealth>(newID, oldID);
    if (player.hasComponent<CCurrency>()) {
        m_ECS->copyComponent<CCurrency>(newID, oldID);
    }
    m_ECS->copyComponent<CState>(newID, oldID);

    if (player.hasComponent<CWeapon>()) {
        m_ECS->copyComponent<CWeapon>(newID, oldID);
    }

    if (!friendly.hasComponent<CInput>()) {
        friendly.addComponent<CInput>();
    }

    m_ECS->queueRemoveEntity(oldID);
    m_scene->changePlayerID(newID);

    return true;
}

void CollisionManager::handlePlayerFriendly(Entity player, Entity friendly, Vec2 overlap)
{
    if (talkToNPC(player, friendly)) {
        return;
    }
    possesNPC(player, friendly);
}

bool CollisionManager::addItemToInventory(Entity player, const Item& item)
{
    if (!player.hasComponent<CInventory>()) {
        return false;
    }

    auto& inventory = player.getComponent<CInventory>();
    auto& activeItem = inventory.activeItem;
    for (int i = 0; i < inventory.size(); ++i) {
        auto& slot = inventory.items[i];
        if (slot.id != -1) {
            continue;
        }

        int index = slot.index;
        slot = item;
        slot.index = index;
        if (index == activeItem.index) {
            m_scene->updateActiveItem(index);
        }
        return true;
    }

    if (!player.hasComponent<CTransform>()) {
        return false;
    }

    const int activeIndex = activeItem.index;
    if (activeIndex < 0 || activeIndex >= inventory.size()) {
        return false;
    }

    Item droppedItem = inventory.items[activeIndex];
    if (droppedItem.id == -1) {
        return false;
    }

    EntityID droppedID = m_scene->DropItem(
        droppedItem,
        player.getComponent<CTransform>().pos
    );
    if (droppedID == static_cast<EntityID>(-1)) {
        return false;
    }

    inventory.items[activeIndex] = item;
    inventory.items[activeIndex].index = activeIndex;
    m_scene->updateActiveItem(activeIndex);
    return true;
}

void CollisionManager::showLootLabel(Entity loot, const std::string& name)
{
    constexpr int LabelLifespan = 16;

    if (loot.hasComponent<CChild>()) {
        for (const auto& childLink : loot.getComponent<CChild>().children) {
            if (!m_ECS->isAlive(childLink.child) || !m_ECS->hasComponent<CText>(childLink.child)) {
                continue;
            }

            CText& label = m_ECS->getComponent<CText>(childLink.child);
            label.text = name;
            if (m_ECS->hasComponent<CLifespan>(childLink.child)) {
                m_ECS->getComponent<CLifespan>(childLink.child).lifespan = LabelLifespan;
            }
            else {
                m_ECS->addComponent<CLifespan>(childLink.child, LabelLifespan);
            }
            return;
        }
    }

    m_scene->SpawnTextBox(
        name,
        12,
        "Minecraft",
        loot.getID(),
        Vec2{0, -24},
        LabelLifespan
    );
}

void CollisionManager::handlePlayerLoot(Entity player, Entity loot, Vec2 overlap)
{
    if (!loot.hasComponent<CName>() || !loot.hasComponent<CItem>()) {
        return;
    }

    std::string name = loot.getComponent<CName>().name;
    int itemID = loot.getComponent<CItem>().itemID;
    Item item = m_scene->getInventoryManager().getItem(itemID);
    CItem& lootItem = loot.getComponent<CItem>();
    const PickupMode pickupMode = lootItem.hasPickupModeOverride
        ? lootItem.pickupModeOverride
        : item.pickupMode;
    const bool isCurrency = item.type == ItemType::Currency;

    if (!isCurrency || pickupMode == PickupMode::Manual) {
        showLootLabel(loot, item.name);
    }

    if (pickupMode == PickupMode::Manual) {
        if (!player.hasComponent<CInput>() || !player.getComponent<CInput>().interact) {
            return;
        }
    }

    if (isCurrency) {
        if (!m_scene->addCurrencyToPlayer(item)) {
            return;
        }
        loot.removeEntity();
        loot.addComponent<CAudio>("loot_pickup");
        if (pickupMode == PickupMode::Manual && player.hasComponent<CInput>()) {
            player.getComponent<CInput>().interact = false;
        }
        return;
    }

    if (!addItemToInventory(player, item)) {
        return;
    }
    loot.removeEntity();
    loot.addComponent<CAudio>("loot_pickup");
    if (player.hasComponent<CInput>()) {
        player.getComponent<CInput>().interact = false;
    }
    m_scene->Emit(Event{EventType::ItemPickedUp, name});
    return;
}

void CollisionManager::handlePlayerArea(Entity player, Entity area, Vec2 overlap)
{
    if (!area.hasComponent<CEvent>()) {
        return;
    }
    Event event = area.getComponent<CEvent>().event;
    m_scene->Emit(event);
    return;
}

void CollisionManager::handleMobWater(Entity mob, Entity water, Vec2 overlap)
{
    mob.getComponent<CVelocity>().vel /= 2;
    return;
}
