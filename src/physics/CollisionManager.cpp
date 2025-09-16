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
    
}

void handleProjectileObstacleCollision(Entity projectile, Entity obstacle, Vec2 overlap){
    // projectile.getComponent<CScript>().Instance->OnDestroyFunction();
    projectile.removeEntity();
}

void handleEnemyProjectileCollision(Entity enemy, Entity projectile, Vec2 overlap){
    projectile.getComponent<CScript>().Instance->OnDestroyFunction();
    projectile.getComponent<CScript>().Instance->OnAttackFunction(enemy.getID());
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

Vec2 BaseCollisionManager::collisionOverlap(CTransform t1, CTransform t2, Vec2 b1, Vec2 b2){    
    Vec2 aSize = b1/2;
    Vec2 bSize = b2/2;
    Vec2 aPos = t1.pos - b1/2;
    Vec2 bPos = t2.pos - b2/2;
    Vec2 aPrevPos = t1.prevPos - b1/2;
    Vec2 bPrevPos = t2.prevPos - b2/2;
    
    Vec2 delta = ( (aPos + aSize) - (bPos + bSize) ).abs_elem();
    Vec2 prevDelta = ( (aPrevPos + aSize) - (bPrevPos + bSize) ).abs_elem();
    Vec2 overlap = aSize + bSize - delta;
    Vec2 prevOverlap = aSize + bSize - prevDelta;
    
    Vec2 move = { 0, 0 };
    if (prevOverlap.y > 0)
    {
        if ((aPos.x + aSize.x) > (bPos.x + bSize.x))
        {
            move += Vec2 { overlap.x, 0.0f };
        }
        if ((aPos.x + aSize.x) < (bPos.x + bSize.x))
        {
            move -= Vec2 { overlap.x, 0.0f };
        }
    }
    
    if (prevOverlap.x > 0)
    {
        if ((aPos.y + aSize.y/2) > (bPos.y + bSize.y/2))
        {
            move +=  Vec2 { 0.0f, overlap.y };
        }
        if ((aPos.y + aSize.y/2) < (bPos.y + bSize.y/2))
        {
            move -=  Vec2 { 0.0f, overlap.y };
        }
    }
    
    return move;
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

CollisionManager::CollisionManager(ECS* ecs, Scene* scene){
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

void CollisionManager::doCollisions(Vec2 treePos, Vec2 treeSize){
    newQuadtree<CCollisionBox>(treePos, treeSize);
    auto& collisionPool = m_ECS->getComponentPool<CCollisionBox>();
    auto& transformPool = m_ECS->getComponentPool<CTransform>();
    
    auto quadVector = m_quadRoot->createQuadtreeVector();
    
    // loop all quadtrees that are not divided
    for (auto quadleaf : quadVector){
        std::vector<Entity> entityVector = quadleaf->getObjects();
        
        // double loop over all entities inside quadtree 
        for (size_t a = 0; a < entityVector.size(); ++a) {
            EntityID entityIDA = entityVector[a].getID();
            auto& collisionA = collisionPool.getComponent(entityIDA);
            
            for (size_t b = a + 1; b < entityVector.size(); ++b) {
                EntityID entityIDB = entityVector[b].getID();
                
                auto& collisionB = collisionPool.getComponent(entityIDB);
                auto& transformA = transformPool.getComponent(entityIDA);
                auto& transformB = transformPool.getComponent(entityIDB);
                auto overlap = collisionOverlap(
                    transformA, 
                    transformB, 
                    collisionA.size, 
                    collisionB.size
                );
                if ( !isCollided(transformA, transformB, collisionA, collisionB) )
                {
                    continue; // No collision detected
                }
                handleCollision(
                    entityIDA, 
                    collisionA.layer, 
                    entityIDB, 
                    collisionB.layer, 
                    overlap
                );
            }
        }
    }
}

void handlePlayerEnemyInteraction(Entity player, Entity enemy, Vec2 overlap){
    std::cout << "handlePlayerEnemyInteraction" << std::endl;
    return;
}

bool talkToNPC(Entity player, Entity friendly){
    if ( !player.hasComponent<CInputs>())
    {
        return false;
    }
    if ( !player.getComponent<CInputs>().interact)
    {
        return false;
    }
    
    std::cout << "Interact" << std::endl;
    return true;
}

bool possesNPC(Entity player, Entity friendly){
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
    
    std::cout << "Possess" << std::endl;
    
    // int possesLevel = friendly.getComponent<CPossesLevel>().level;
    // friendly.removeComponent<CPossesLevel>();

    // m_scene->changePlayerID(friendly);
    // friendly.addComponent<CInputs>();
    // friendly.copyComponent<CCollisionBox>(friendly, player);
    // friendly.copyComponent<CInteractionBox>(friendly, player);

    // player.printEntityComponents(player);
    
    // player.printEntityComponents(friendly);

    // player.removeEntity(player);
    return true;
}

void handlePlayerFriendlyInteraction(Entity player, Entity friendly, Vec2 overlap){
    if (talkToNPC(player, friendly)){
        return;
    }
    possesNPC(player, friendly);
    return;
}

void handlePlayerLootInteraction(Entity player, Entity loot, Vec2 overlap){
    loot.removeEntity();
    loot.addComponent<CAudio>("loot_pickup");
    return;
}

InteractionManager::InteractionManager(ECS* ecs, Scene* scene){
    m_ECS = ecs;
    m_scene = scene;
    registerHandler(PLAYER_LAYER, ENEMY_LAYER, handlePlayerEnemyInteraction);
    registerHandler(PLAYER_LAYER, FRIENDLY_LAYER, handlePlayerFriendlyInteraction);
    registerHandler(PLAYER_LAYER, LOOT_LAYER, handlePlayerLootInteraction);
}

void InteractionManager::doInteractions(Vec2 treePos, Vec2 treeSize){
    auto& interactionPool = m_ECS->getComponentPool<CInteractionBox>();
    auto& transformPool = m_ECS->getComponentPool<CTransform>();

    newQuadtree<CInteractionBox>(treePos, treeSize);
    auto quadVector = m_quadRoot->createQuadtreeVector();

    for (auto quadleaf : quadVector){
        std::vector<Entity> entityVector = quadleaf->getObjects();
        for (size_t a = 0; a < entityVector.size(); ++a){
            EntityID idA = entityVector[a].getID();
            auto& interactionA = interactionPool.getComponent(idA);
            auto& transformA = transformPool.getComponent(idA);
            
            for (size_t b = a + 1; b < entityVector.size(); ++b) {
                EntityID idB = entityVector[b].getID();
                if ( idA == idB ) {
                    continue; // Skip self-collision
                }
                auto& interactionB = interactionPool.getComponent(idB);
                auto& transformB = transformPool.getComponent(idB);
                if ( !isCollided(transformA, transformB, interactionA, interactionB) ){
                    continue; // No collision detected
                }
                bool BLayer = (interactionB.layer & interactionA.mask) != interactionB.layer;
                bool ALayer = (interactionA.layer & interactionB.mask) != interactionA.layer;
                if ( BLayer || ALayer){
                    continue; // No interaction layer match
                }
                
                if (m_ECS->hasComponent<CName>(idA)){
                    std::string nameA = m_ECS->getComponent<CName>(idA).name;
                    m_ECS->addComponent<CEvent>(idA, Event{EventType::EnteredArea, nameA});
                }
                if (m_ECS->hasComponent<CName>(idB)){
                    std::string nameB = m_ECS->getComponent<CName>(idB).name;
                    m_ECS->addComponent<CEvent>(idB, Event{EventType::EnteredArea, nameB});
                }

                handleCollision(
                    idA, 
                    interactionA.layer, 
                    idB, 
                    interactionB.layer, 
                    {0,0}
                );
                
                // if (scriptPool.hasComponent(idA)){
                //     auto& scriptA = scriptPool.getComponent(idA);
                //     scriptA.Instance->OnInteractFunction(idB, interactionB.layer);
                //     scriptA.Instance->onPosses(idB, idA);
                // }
                
                // if (scriptPool.hasComponent(idB)){
                //     auto& scriptB = scriptPool.getComponent(idB);
                //     scriptB.Instance->OnInteractFunction(idA, interactionA.layer);
                //     scriptB.Instance->onPosses(idA, idB);                
                // }
            }
        }
    }
}
