#include <cmath>

#include "CollisionManager.h"
#include "ScriptableEntity.h"

void handlePlayerObstacleCollision(Entity player, Entity obstacle, Vec2 overlap)
{
    std::cout << "Player collided with obstacle: " << obstacle.getID() << std::endl;
    player.getComponent<CTransform>().pos += overlap;
}

void handlePlayerEnemyCollision(Entity player, Entity enemy, Vec2 overlap)
{
    std::cout << "Player collided with enemy: " << enemy.getID() << std::endl;
    player.getComponent<CTransform>().pos += overlap/2;
    enemy.getComponent<CTransform>().pos -= overlap/2;

}

void handleFriendlyObstacleCollision(Entity friendly, Entity obstacle, Vec2 overlap)
{
    std::cout << "Friendly: " << friendly.getID() << " collided with obstacle: " << obstacle.getID() << std::endl;
}

void handleProjectileObstacleCollision(Entity projectile, Entity obstacle, Vec2 overlap)
{
    std::cout << "Projectile: " << projectile.getID() << " collided with obstacle: " << obstacle.getID() << std::endl;
}

void handleEnemyProjectileCollision(Entity enemy, Entity projectile, Vec2 overlap)
{
    std::cout << "Enemy: " << enemy.getID() << " collided with projectile: " << projectile.getID() << std::endl;
}

void handleEnemyEnemyCollision(Entity enemyA, Entity enemyB, Vec2 overlap)
{
    std::cout << "EnemyA: " << enemyA.getID() << " collided with enemyB: " << enemyB.getID() << std::endl;
    enemyA.getComponent<CTransform>().pos -= overlap/2;
    enemyB.getComponent<CTransform>().pos += overlap/2;
}


void handleEnemyObstacleCollision(Entity enemy, Entity obstacle, Vec2 overlap)
{
    std::cout << "Enemy: " << enemy.getID() << " collided with obstacle: " << obstacle.getID() << std::endl;
    enemy.getComponent<CTransform>().pos -= overlap;
}

CollisionManager::CollisionManager(ECS* ecs, Scene* scene)
{
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

void CollisionManager::registerHandler(CollisionMask layerA, CollisionMask layerB, CollisionHandler handler)
{
    uint8_t indexA = std::log2((uint8_t)layerA.to_ulong());
    uint8_t indexB = std::log2((uint8_t)layerB.to_ulong());
    std::cout << "Registering handler for layers: " 
              << (int)indexA << " and " << (int)indexB << std::endl;
    
    if (indexA > indexB) {
        std::swap(indexA, indexB);
    }
    m_handlerMatrix[indexA][indexB] = handler;
}

void CollisionManager::newQuadtree(Vec2 pos, Vec2 size)
{
    m_quadRoot = std::make_unique<Quadtree>(pos, size);

    auto viewCollision = m_ECS->signatureView<CCollisionBox, CTransform>();
    for ( auto e : viewCollision ){
        Entity entity = {e, m_ECS};
        m_quadRoot->insert<CCollisionBox>(entity);
    }
}

void CollisionManager::newInteractionQuadtree(Vec2 pos, Vec2 size)
{
    m_interactionQuadRoot = std::make_unique<Quadtree>(pos, size);

    auto viewCollision = m_ECS->signatureView<CInteractionBox, CTransform>();
    for ( auto e : viewCollision ){
        Entity entity = {e, m_ECS};
        m_interactionQuadRoot->insert<CInteractionBox>(entity);
    }
}

void CollisionManager::doCollisions()
{
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
                auto& collisionLayerA = collisionA.layer;
                auto& collisionMaskA = collisionA.mask;
                auto& collisionLayerB = collisionB.layer;
                auto& collisionMaskB = collisionB.mask;
                if ( ((collisionLayerB & collisionMaskA) != collisionLayerB) | 
                     ((collisionLayerA & collisionMaskB) != collisionLayerA) ){
                    continue; // No collision layer match
                }
                auto& transformA = transformPool.getComponent(entityIDA);
                auto& transformB = transformPool.getComponent(entityIDB);
                auto overlap = collisionOverlap(transformA, transformB, collisionA.size, collisionB.size);
                overlap.print("overlap: ");
                if ( !isCollided(transformA, transformB, collisionA, collisionB) )
                {
                    continue; // No collision detected
                }
                
                handleCollision(entityIDA, collisionLayerA, entityIDB, collisionLayerB, overlap);
            }
        }
    }
}

void CollisionManager::handleCollision(
    EntityID entityIDA, CollisionMask layerA, 
    EntityID entityIDB, CollisionMask layerB, Vec2 overlap)
{
    uint8_t indexA = std::log2((uint8_t)layerA.to_ulong());
    uint8_t indexB = std::log2((uint8_t)layerB.to_ulong());
    
    if (!m_handlerMatrix[indexA][indexB])
    {
        return;
    }
    // Ensure entityA always corresponds to the smaller index for handler signature consistency
    if (indexA > indexB) {
        std::swap(entityIDA, entityIDB);
        std::swap(indexA, indexB);
        overlap*=-1;
    }

    Entity entityA = {entityIDA, m_ECS};
    Entity entityB = {entityIDB, m_ECS};

    m_handlerMatrix[indexA][indexB](entityA, entityB, overlap);
}

Vec2 CollisionManager::collisionOverlap(CTransform t1, CTransform t2, Vec2 b1, Vec2 b2)
{    
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
            move += Vec2 { overlap.x, 0 };
        }
        if ((aPos.x + aSize.x) < (bPos.x + bSize.x))
        {
            move -= Vec2 { overlap.x, 0 };
        }
    }
    
    if (prevOverlap.x > 0)
    {
        if ((aPos.y + aSize.y/2) > (bPos.y + bSize.y/2))
        {
            move +=  Vec2 { 0, overlap.y };
        }
        if ((aPos.y + aSize.y/2) < (bPos.y + bSize.y/2))
        {
            move -=  Vec2 { 0, overlap.y };
        }
    }
    
    // if (move.hasNegative())
    // {
    //     return Vec2{0, 0};
    // }
    // else{
    // }
    
    return move;
}

bool CollisionManager::isCollided(CTransform t1, CTransform t2, CCollisionBox b1, CCollisionBox b2)
{
    Vec2 aSize = b1.size;
    Vec2 bSize = b2.size;
    Vec2 aPos = t1.pos - b1.halfSize;
    Vec2 bPos = t2.pos - b2.halfSize;

    bool x_overlap = (aPos.x + aSize.x > bPos.x) && (bPos.x + bSize.x > aPos.x);
    bool y_overlap = (aPos.y + aSize.y > bPos.y) && (bPos.y + bSize.y > aPos.y);

    return (x_overlap && y_overlap);
}

void CollisionManager::renderQuadtree(SDL_Renderer* renderer, int zoom, Vec2 center, Vec2 cameraPosition)
{
    SDL_Color color = {255, 0, 0, 255};
    m_quadRoot->renderBoundary(renderer, zoom, center, cameraPosition, color);
}

void CollisionManager::renderInteractionQuadtree(SDL_Renderer* renderer, int zoom, Vec2 center, Vec2 cameraPosition)
{
    SDL_Color color = {0, 0, 255, 255};
    m_interactionQuadRoot->renderBoundary(renderer, zoom, center, cameraPosition, color);
}