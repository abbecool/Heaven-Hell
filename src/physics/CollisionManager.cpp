#include <cmath>

#include "physics/CollisionManager.h"
#include "ecs/ScriptableEntity.h"

void handlePlayerObstacleCollision(Entity player, Entity obstacle, Vec2 overlap){
    player.getComponent<CTransform>().pos += overlap;
}

void handlePlayerEnemyCollision(Entity player, Entity enemy, Vec2 overlap){
    player.getComponent<CTransform>().pos += overlap/2;
    enemy.getComponent<CTransform>().pos -= overlap/2;
}

void handleFriendlyObstacleCollision(Entity friendly, Entity obstacle, Vec2 overlap){
    friendly.getComponent<CTransform>().pos += overlap;
}

void handleProjectileObstacleCollision(Entity projectile, Entity obstacle, Vec2 overlap){
    projectile.removeEntity();
}

void handleEnemyProjectileCollision(Entity enemy, Entity projectile, Vec2 overlap){
    int damage = projectile.getComponent<CDamage>().damage;
    enemy.getComponent<CHealth>().HP -= damage;
    projectile.removeEntity();
}

void handleEnemyEnemyCollision(Entity enemyA, Entity enemyB, Vec2 overlap){
    enemyA.getComponent<CTransform>().pos += overlap/2;
    enemyB.getComponent<CTransform>().pos -= overlap/2;
}


void handleEnemyObstacleCollision(Entity enemy, Entity obstacle, Vec2 overlap){
    enemy.getComponent<CTransform>().pos -= overlap;
}

void BaseCollisionManager::registerHandler(
    CollisionMask layerA, 
    CollisionMask layerB, 
    Handler handler
){
    uint8_t indexA = std::log2((uint8_t)layerA.to_ulong());
    uint8_t indexB = std::log2((uint8_t)layerB.to_ulong());
    
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
    uint8_t indexA = std::log2((uint8_t)layerA.to_ulong());
    uint8_t indexB = std::log2((uint8_t)layerB.to_ulong());
    
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

void BaseCollisionManager::renderQuadtree(
    SDL_Renderer* renderer, 
    int zoom, 
    Vec2 center, 
    Vec2 cameraPosition
){
    SDL_Color color = {255, 0, 0, 255};
    m_quadRoot->renderBoundary(renderer, zoom, center, cameraPosition, color);
}

CollisionManager::CollisionManager(ECS* ecs, Scene_Play* scene){
    m_ECS = ecs;
    m_scene = scene;
    registerHandler(PLAYER_LAYER, OBSTACLE_LAYER, handlePlayerObstacleCollision);
    registerHandler(PLAYER_LAYER, ENEMY_LAYER, handlePlayerEnemyCollision);
    registerHandler(PLAYER_LAYER, FRIENDLY_LAYER, handlePlayerObstacleCollision);
    registerHandler(FRIENDLY_LAYER, OBSTACLE_LAYER, handleFriendlyObstacleCollision);
    registerHandler(ENEMY_LAYER, PROJECTILE_LAYER, handleEnemyProjectileCollision);
    registerHandler(ENEMY_LAYER, ENEMY_LAYER, handleEnemyEnemyCollision);
    registerHandler(ENEMY_LAYER, OBSTACLE_LAYER, handleEnemyObstacleCollision);
    registerHandler(PROJECTILE_LAYER, OBSTACLE_LAYER, handleProjectileObstacleCollision);
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
    if (!enemy.hasComponent<CWeapon>()){
        return;
    }
    int damage = enemy.getComponent<CWeapon>().damage;
    Vec2 position = enemy.getComponent<CTransform>().pos;
    Vec2 playerPosition = player.getComponent<CTransform>().pos;
    m_scene->spawnHitbox(position, position-playerPosition);
    return;
}

bool InteractionManager::talkToNPC(Entity player, Entity friendly){

    if (!player.hasComponent<CInputs>())
    {
        return false;
    }
    if (!player.getComponent<CInputs>().interact)
    {
        return false;
    }
    
    int currentQuestID = m_scene->getStoryManager().getCurrentQuestID();
    std::string name = friendly.getComponent<CName>().name;
    std::string currentDialog = m_scene->getStoryManager().getDialog(name);
    if (!friendly.hasComponent<CChild>()) {
        m_scene->SpawnDialog(currentDialog, 16, "Minecraft", friendly.getID());
    }
    m_scene->Emit(Event{EventType::DialogueFinished, name});
    return true;
} 

bool InteractionManager::possesNPC(Entity player, Entity friendly){
    
    if (!player.hasComponent<CInputs>())
    {
        return false;
    }
    if (!player.getComponent<CInputs>().posses)
    {
        return false;
    }
    if (!friendly.hasComponent<CPossesLevel>())
    {
        return false;
    }

    int possesLevel = friendly.getComponent<CPossesLevel>().level;
    friendly.removeComponent<CPossesLevel>();

    m_scene->changePlayerID(friendly.getID());
    friendly.addComponent<CInputs>();
    m_ECS->copyComponent<CCollisionBox>(friendly.getID(), player.getID());
    m_ECS->copyComponent<CInteractionBox>(friendly.getID(), player.getID());

    m_ECS->printEntityComponents(player.getID());
    m_ECS->printEntityComponents(friendly.getID());

    player.removeEntity();
    return true;
}

void InteractionManager::handlePlayerFriendly(Entity player, Entity friendly, Vec2 overlap){
    if (talkToNPC(player, friendly)){
        return;
    }
    possesNPC(player, friendly);
    return;
}

void InteractionManager::handlePlayerLoot(Entity player, Entity loot, Vec2 overlap){
    loot.removeEntity();
    loot.addComponent<CAudio>("loot_pickup");
    if (!loot.hasComponent<CName>()){
        return;
    }
    std::string name = loot.getComponent<CName>().name;
    int itemID = loot.getComponent<CItem>().itemID;
    Item item = m_scene->getInventoryManager().getItem(itemID);
    auto& inventory = player.getComponent<CInventory>();
    auto& activeItem = inventory.activeItem;
    for (auto& slot : inventory.items) {
        if (slot.id != -1) { // Assuming -1 means empty slot
            continue;
        }
        int index = slot.index;
        slot = item;
        slot.index = index;
        if (index == activeItem.index){
            m_scene->updateActiveItem(index);
        }
        break;
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
    // registerHandler(PLAYER_LAYER, AREA_LAYER, 
    //     [this](Entity a, Entity b, Vec2 overlap) {handlePlayerArea(a, b, overlap);}
    // );
}

bool InteractionManager::checkInteractionLayerMask(const CInteractionBox& boxA, const CInteractionBox& boxB) const {
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
