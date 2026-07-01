#include <cmath>

#include "physics/CollisionManager.hpp"
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

} // namespace

void BaseCollisionManager::registerHandler(     
    CollisionMask layerA, 
    CollisionMask layerB, 
    Handler handler
){
    uint8_t indexA = static_cast<uint8_t>(std::log2(static_cast<uint8_t>(layerA.to_ulong())));
    uint8_t indexB = static_cast<uint8_t>(std::log2(static_cast<uint8_t>(layerB.to_ulong())));
    
    if (indexA > indexB) {
        std::swap(indexA, indexB);
    }
    m_handlerMatrix[indexA][indexB] = handler;
}

template <typename T>
void BaseCollisionManager::newQuadtree(Vec2 pos, Vec2 size){
    auto view = m_ECS->View<T, CTransform>();
    auto& transformPool = m_ECS->getComponentPool<CTransform>();
    auto& TPool = m_ECS->getComponentPool<T>();

    m_quadRoot = std::make_unique<Quadtree>(pos, size);
    for ( auto e : view ){
        Entity entity = {e, m_ECS};
        auto entityPos = transformPool.getComponent(e).pos;
        auto entitySize = TPool.getComponent(e).size;
        m_quadRoot->insert1<T>(entity, entityPos, entitySize);
    }
}

void BaseCollisionManager::handleCollision(
    EntityID entityIDA, CollisionMask layerA, 
    EntityID entityIDB, CollisionMask layerB, Vec2 overlap){
    uint8_t indexA = static_cast<uint8_t>(std::log2(static_cast<uint8_t>(layerA.to_ulong())));
    uint8_t indexB = static_cast<uint8_t>(std::log2(static_cast<uint8_t>(layerB.to_ulong())));
    
    // Ensure entityA always corresponds to the smaller index for handler signature consistency
    if (indexA > indexB) {
        std::swap(entityIDA, entityIDB);
        std::swap(indexA, indexB);
        overlap*=-1;
    }
    
    Entity entityA = {entityIDA, m_ECS};
    Entity entityB = {entityIDB, m_ECS};
    
    if (m_handlerMatrix[indexA][indexB])
    {
        m_handlerMatrix[indexA][indexB](entityA, entityB, overlap);
    }
}

Vec2 BaseCollisionManager::calculateDelta(Vec2 aPos, Vec2 aSize, Vec2 bPos, Vec2 bSize) const {
    return ( (aPos + aSize) - (bPos + bSize) ).abs_elem();
}

Vec2 BaseCollisionManager::calculateHorizontalMovement(
    const Vec2& aPos, const Vec2& aSize, 
    const Vec2& bPos, const Vec2& bSize, 
    const Vec2& overlap, const Vec2& prevOverlap) const 
{
    Vec2 move = { 0, 0 };
    if (prevOverlap.y <= 0) {
        return move;
    }
    
    if ((aPos.x + aSize.x) > (bPos.x + bSize.x)) {
        move += Vec2 { overlap.x, 0.0f };
    }
    if ((aPos.x + aSize.x) < (bPos.x + bSize.x)) {
        move -= Vec2 { overlap.x, 0.0f };
    }
    return move;
}

Vec2 BaseCollisionManager::calculateVerticalMovement(
    const Vec2& aPos, const Vec2& aSize, 
    const Vec2& bPos, const Vec2& bSize, 
    const Vec2& overlap, const Vec2& prevOverlap) const 
{
    Vec2 move = { 0, 0 };
    if (prevOverlap.x <= 0) {
        return move;
    }
    
    if ((aPos.y + aSize.y/2) > (bPos.y + bSize.y/2)) {
        move += Vec2 { 0.0f, overlap.y };
    }
    if ((aPos.y + aSize.y/2) < (bPos.y + bSize.y/2)) {
        move -= Vec2 { 0.0f, overlap.y };
    }
    return move;
}

Vec2 BaseCollisionManager::collisionOverlap(CTransform t1, CTransform t2, Vec2 b1, Vec2 b2){    
    Vec2 aSize = b1/2;
    Vec2 bSize = b2/2;
    Vec2 aPos = t1.pos - b1/2;
    Vec2 bPos = t2.pos - b2/2;
    Vec2 aPrevPos = t1.prevPos - b1/2;
    Vec2 bPrevPos = t2.prevPos - b2/2;
    
    Vec2 delta = calculateDelta(aPos, aSize, bPos, bSize);
    Vec2 prevDelta = calculateDelta(aPrevPos, aSize, bPrevPos, bSize);
    Vec2 overlap = aSize + bSize - delta;
    Vec2 prevOverlap = aSize + bSize - prevDelta;
    
    Vec2 horizontalMove = calculateHorizontalMovement(aPos, aSize, bPos, bSize, overlap, prevOverlap);
    Vec2 verticalMove = calculateVerticalMovement(aPos, aSize, bPos, bSize, overlap, prevOverlap);
    
    return horizontalMove + verticalMove;
}

bool BaseCollisionManager::isCollided(
    CTransform t1, 
    CTransform t2, 
    CCollisionBox b1, 
    CCollisionBox b2
){
    Vec2 aSize = b1.size;
    Vec2 bSize = b2.size;
    Vec2 aPos = t1.pos - b1.halfSize;
    Vec2 bPos = t2.pos - b2.halfSize;

    bool x_overlap = (aPos.x + aSize.x > bPos.x) && (bPos.x + bSize.x > aPos.x);
    bool y_overlap = (aPos.y + aSize.y > bPos.y) && (bPos.y + bSize.y > aPos.y);

    return (x_overlap && y_overlap);
}

bool BaseCollisionManager::isCollided(
    CTransform t1, 
    CTransform t2, 
    CInteractionBox b1, 
    CInteractionBox b2){
    Vec2 aSize = b1.size;
    Vec2 bSize = b2.size;
    Vec2 aPos = t1.pos - b1.halfSize;
    Vec2 bPos = t2.pos - b2.halfSize;

    bool x_overlap = (aPos.x + aSize.x > bPos.x) && (bPos.x + bSize.x > aPos.x);
    bool y_overlap = (aPos.y + aSize.y > bPos.y) && (bPos.y + bSize.y > aPos.y);

    return (x_overlap && y_overlap);
}

bool BaseCollisionManager::isCollided(CTransform t1, CTransform t2, CBox b1, CBox b2){
    Vec2 aSize = b1.size;
    Vec2 bSize = b2.size;
    Vec2 aPos = t1.pos - b1.halfSize;
    Vec2 bPos = t2.pos - b2.halfSize;

    bool x_overlap = (aPos.x + aSize.x > bPos.x) && (bPos.x + bSize.x > aPos.x);
    bool y_overlap = (aPos.y + aSize.y > bPos.y) && (bPos.y + bSize.y > aPos.y);

    return (x_overlap && y_overlap);
}

void BaseCollisionManager::renderQuadtree(RenderBackend& renderer){
    Color color = {255, 0, 0, 255};
    m_quadRoot->renderBoundary(renderer, color);
}

CollisionManager::CollisionManager(ECS* ecs, Scene_Play* scene){
    m_ECS = ecs;
    m_scene = scene;
    registerHandler(PLAYER_LAYER, OBSTACLE_LAYER, handleBodyCollision);
    registerHandler(PLAYER_LAYER, ENEMY_LAYER, handleBodyCollision);
    registerHandler(PLAYER_LAYER, FRIENDLY_LAYER, handleBodyCollision);
    registerHandler(FRIENDLY_LAYER, OBSTACLE_LAYER, handleBodyCollision);
    registerHandler(FRIENDLY_LAYER, FRIENDLY_LAYER, handleBodyCollision);
    registerHandler(FRIENDLY_LAYER, ENEMY_LAYER, handleBodyCollision);
    registerHandler(
        ENEMY_LAYER,
        PROJECTILE_LAYER,
        [this](Entity enemy, Entity projectile, Vec2 overlap) {
            if (!projectile.hasComponent<CProjectileState>() ||
                projectile.getComponent<CProjectileState>().phase != ProjectilePhase::Flying) {
                return;
            }

            int damage = projectile.getComponent<CDamage>().damage;
            enemy.getComponent<CHealth>().HP -= damage;
            m_scene->destroyProjectile(projectile.getID());
        }
    );
    registerHandler(ENEMY_LAYER, ENEMY_LAYER, handleBodyCollision);
    registerHandler(ENEMY_LAYER, OBSTACLE_LAYER, handleBodyCollision);
    registerHandler(
        PROJECTILE_LAYER,
        OBSTACLE_LAYER,
        [this](Entity projectile, Entity obstacle, Vec2 overlap) {
            if (!projectile.hasComponent<CProjectileState>() ||
                projectile.getComponent<CProjectileState>().phase != ProjectilePhase::Flying) {
                return;
            }

            m_scene->destroyProjectile(projectile.getID());
        }
    );
}

void CollisionManager::processQuadtreeLeaf(std::vector<Entity>& entityVector) {
    auto& collisionPool = m_ECS->getComponentPool<CCollisionBox>();
    auto& transformPool = m_ECS->getComponentPool<CTransform>();
    
    // Double loop over all entities inside this quadtree leaf
    for (size_t a = 0; a < entityVector.size(); ++a) {
        EntityID entityIDA = entityVector[a].getID();
        auto& collisionA = collisionPool.getComponent(entityIDA);
        auto& transformA = transformPool.getComponent(entityIDA);
        
        for (size_t b = a + 1; b < entityVector.size(); ++b) {
            EntityID entityIDB = entityVector[b].getID();
            auto& collisionB = collisionPool.getComponent(entityIDB);
            auto& transformB = transformPool.getComponent(entityIDB);
            
            // Check if entities actually collide
            if (!isCollided(transformA, transformB, collisionA, collisionB)) {
                continue;
            }
            
            // Calculate overlap and handle collision
            auto overlap = collisionOverlap(transformA, transformB, collisionA.size, collisionB.size);
            handleCollision(entityIDA, collisionA.layer, entityIDB, collisionB.layer, overlap);
        }
    }
}

void CollisionManager::doCollisions(Vec2 treePos, Vec2 treeSize){
    newQuadtree<CCollisionBox>(treePos, treeSize);
    auto quadVector = m_quadRoot->createQuadtreeVector();
    
    // Process all quadtree leaves
    for (auto quadleaf : quadVector) {
        std::vector<Entity> entityVector = quadleaf->getObjects();
        processQuadtreeLeaf(entityVector);
    }
}

void InteractionManager::handlePlayerEnemy(Entity player, Entity enemy, Vec2 overlap){
    if (!enemy.hasComponent<CWeapon>() || !enemy.hasComponent<CInput>()){
        return;
    }
    CInput& input = enemy.getComponent<CInput>();
    CWeapon& weapon = enemy.getComponent<CWeapon>();
    if (weapon.delay <= 0){
        input.attack = true;
    }
    return;
}

void InteractionManager::handleDamageHitbox(Entity entityA, Entity entityB, Vec2 overlap)
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
    target.getComponent<CHealth>().HP -= hitbox.getComponent<CDamage>().damage;
}

bool InteractionManager::talkToNPC(Entity player, Entity friendly){

    if (!player.hasComponent<CInput>())
    {
        return false;
    }
    if (!player.getComponent<CInput>().interact)
    {
        return false;
    }
    
    int currentQuestID = m_scene->getStoryManager().getCurrentQuestID();
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

bool InteractionManager::possesNPC(Entity player, Entity friendly){
    if (!player.hasComponent<CInput>())          { return false; }
    if (!player.getComponent<CInput>().posses)   { return false; }
    if (!friendly.hasComponent<CPossesLevel>())  { return false; }

    friendly.removeComponent<CPossesLevel>();
    EntityID oldID = player.getID();
    EntityID newID = friendly.getID();

    // 1. Transfer all components the systems depend on BEFORE changing m_player
    m_ECS->copyComponent<CCollisionBox>(newID, oldID);
    m_ECS->copyComponent<CInteractionBox>(newID, oldID);
    m_ECS->copyComponent<CInventory>(newID, oldID);
    m_ECS->copyComponent<CHealth>(newID, oldID);
    if (player.hasComponent<CCurrency>()) {
        m_ECS->copyComponent<CCurrency>(newID, oldID);
    }
    m_ECS->copyComponent<CState>(newID, oldID);

    // Copy weapon only if old player had one
    if (player.hasComponent<CWeapon>()) {
        m_ECS->copyComponent<CWeapon>(newID, oldID);
    }

    // 2. Give the new player an input component
    if (!friendly.hasComponent<CInput>()) {
        friendly.addComponent<CInput>();
    }

    // 3. Queue the old player for removal BEFORE updating m_player
    //    so no system tries to read m_player = newID while oldID still exists
    m_ECS->queueRemoveEntity(oldID);

    // 4. Update m_player last, after everything is set up
    m_scene->changePlayerID(newID);

    return true;
}

void InteractionManager::handlePlayerFriendly(Entity player, Entity friendly, Vec2 overlap){
    if (talkToNPC(player, friendly)){
        return;
    }
    possesNPC(player, friendly);
    return;
}

bool InteractionManager::addItemToInventory(Entity player, const Item& item)
{
    if (!player.hasComponent<CInventory>()) {
        return false;
    }

    auto& inventory = player.getComponent<CInventory>();
    auto& activeItem = inventory.activeItem;
    for (auto& slot : inventory.items) {
        if (slot.id != -1) {
            continue;
        }

        int index = slot.index;
        slot = item;
        slot.index = index;
        if (index == activeItem.index){
            m_scene->updateActiveItem(index);
        }
        return true;
    }

    if (!player.hasComponent<CTransform>()) {
        return false;
    }

    const int activeIndex = activeItem.index;
    if (activeIndex < 0 || activeIndex >= static_cast<int>(inventory.items.size())) {
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

void InteractionManager::showLootLabel(Entity loot, const std::string& name)
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

void InteractionManager::handlePlayerLoot(Entity player, Entity loot, Vec2 overlap){
    if (!loot.hasComponent<CName>() || !loot.hasComponent<CItem>()){
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

void InteractionManager::handlePlayerArea(Entity player, Entity area, Vec2 overlap){
    if (!area.hasComponent<CEvent>()){
        return;
    }
    Event event = area.getComponent<CEvent>().event;
    m_scene->Emit(event);
    return;
}

InteractionManager::InteractionManager(ECS* ecs, Scene_Play* scene){
    m_ECS = ecs;
    m_scene = scene;
    registerHandler(PLAYER_LAYER, ENEMY_LAYER, 
        [this](Entity a, Entity b, Vec2 overlap) {handlePlayerEnemy(a, b, overlap);}
    );
    registerHandler(PLAYER_LAYER, FRIENDLY_LAYER, 
        [this](Entity a, Entity b, Vec2 overlap) {handlePlayerFriendly(a, b, overlap);}
    );
    registerHandler(PLAYER_LAYER, LOOT_LAYER, 
        [this](Entity a, Entity b, Vec2 overlap) {handlePlayerLoot(a, b, overlap);}
    );
    registerHandler(PLAYER_LAYER, AREA_LAYER, 
        [this](Entity a, Entity b, Vec2 overlap) {handlePlayerArea(a, b, overlap);}
    );
    registerHandler(DAMAGE_LAYER, ENEMY_LAYER,
        [this](Entity a, Entity b, Vec2 overlap) {handleDamageHitbox(a, b, overlap);}
    );
    registerHandler(DAMAGE_LAYER, PLAYER_LAYER,
        [this](Entity a, Entity b, Vec2 overlap) {handleDamageHitbox(a, b, overlap);}
    );
    // registerHandler(PLAYER_LAYER, AREA_LAYER, 
    //     [this](Entity a, Entity b, Vec2 overlap) {handlePlayerArea(a, b, overlap);}
    // );
}

bool InteractionManager::checkInteractionLayerMask(const CInteractionBox& boxA, const CInteractionBox& boxB) const {
    const bool aIsDamage = (boxA.layer & DAMAGE_LAYER) == DAMAGE_LAYER;
    const bool bIsDamage = (boxB.layer & DAMAGE_LAYER) == DAMAGE_LAYER;

    if (aIsDamage && !bIsDamage) {
        return (boxB.layer & boxA.mask) != boxB.layer;
    }
    if (bIsDamage && !aIsDamage) {
        return (boxA.layer & boxB.mask) != boxA.layer;
    }

    bool BLayer = (boxB.layer & boxA.mask) != boxB.layer;
    bool ALayer = (boxA.layer & boxB.mask) != boxA.layer;
    return BLayer || ALayer;
}

void InteractionManager::processInteractionLeaf(std::vector<Entity>& entityVector) {
    auto& interactionPool = m_ECS->getComponentPool<CInteractionBox>();
    auto& transformPool = m_ECS->getComponentPool<CTransform>();
    
    for (size_t a = 0; a < entityVector.size(); ++a) {
        EntityID idA = entityVector[a].getID();
        auto& interactionA = interactionPool.getComponent(idA);
        auto& transformA = transformPool.getComponent(idA);
        
        for (size_t b = a + 1; b < entityVector.size(); ++b) {
            EntityID idB = entityVector[b].getID();
            
            // Skip self-collision
            if (idA == idB) {
                continue;
            }
            
            auto& interactionB = interactionPool.getComponent(idB);
            auto& transformB = transformPool.getComponent(idB);
            
            // Check if interaction boxes collide
            if (!isCollided(transformA, transformB, interactionA, interactionB)) {
                continue;
            }
            
            // Check layer mask compatibility
            if (checkInteractionLayerMask(interactionA, interactionB)) {
                continue;
            }
            
            // Handle the interaction
            handleCollision(idA, interactionA.layer, idB, interactionB.layer, {0, 0});
        }
    }
}

void InteractionManager::doInteractions(Vec2 treePos, Vec2 treeSize){
    newQuadtree<CInteractionBox>(treePos, treeSize);
    auto quadVector = m_quadRoot->createQuadtreeVector();

    for (auto quadleaf : quadVector) {
        std::vector<Entity> entityVector = quadleaf->getObjects();
        processInteractionLeaf(entityVector);
    }
}
