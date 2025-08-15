#include <cmath>

#include "physics/CollisionManager.h"
#include "ecs/ScriptableEntity.h"

void handlePlayerObstacleCollision(Entity player, Entity obstacle, Vec2 overlap)
{
    player.getComponent<CTransform>().pos += overlap;
}

void handlePlayerEnemyCollision(Entity player, Entity enemy, Vec2 overlap)
{
    player.getComponent<CTransform>().pos += overlap/2;
    enemy.getComponent<CTransform>().pos -= overlap/2;

}

void handleFriendlyObstacleCollision(Entity friendly, Entity obstacle, Vec2 overlap)
{
}

void handleProjectileObstacleCollision(Entity projectile, Entity obstacle, Vec2 overlap)
{
    projectile.getComponent<CScript>().Instance->OnDestroyFunction();
}

void handleEnemyProjectileCollision(Entity enemy, Entity projectile, Vec2 overlap)
{
    projectile.getComponent<CScript>().Instance->OnDestroyFunction();
    projectile.getComponent<CScript>().Instance->OnAttackFunction(enemy.getID());
}

void handleEnemyEnemyCollision(Entity enemyA, Entity enemyB, Vec2 overlap)
{
    enemyA.getComponent<CTransform>().pos += overlap/2;
    enemyB.getComponent<CTransform>().pos -= overlap/2;
}


void handleEnemyObstacleCollision(Entity enemy, Entity obstacle, Vec2 overlap)
{
    enemy.getComponent<CTransform>().pos -= overlap;
}

void BaseCollisionManager::registerHandler(CollisionMask layerA, CollisionMask layerB, Handler handler)
{
    uint8_t indexA = std::log2((uint8_t)layerA.to_ulong());
    uint8_t indexB = std::log2((uint8_t)layerB.to_ulong());
    
    if (indexA > indexB) {
        std::swap(indexA, indexB);
    }
    m_handlerMatrix[indexA][indexB] = handler;
}

template <typename T>
void BaseCollisionManager::newQuadtree(Vec2 pos, Vec2 size)
{
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
    EntityID entityIDB, CollisionMask layerB, Vec2 overlap)
{
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

Vec2 BaseCollisionManager::collisionOverlap(CTransform t1, CTransform t2, Vec2 b1, Vec2 b2)
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
    
    return move;
}

bool BaseCollisionManager::isCollided(CTransform t1, CTransform t2, CCollisionBox b1, CCollisionBox b2)
{
    Vec2 aSize = b1.size;
    Vec2 bSize = b2.size;
    Vec2 aPos = t1.pos - b1.halfSize;
    Vec2 bPos = t2.pos - b2.halfSize;

    bool x_overlap = (aPos.x + aSize.x > bPos.x) && (bPos.x + bSize.x > aPos.x);
    bool y_overlap = (aPos.y + aSize.y > bPos.y) && (bPos.y + bSize.y > aPos.y);

    return (x_overlap && y_overlap);
}

bool BaseCollisionManager::isCollided(CTransform t1, CTransform t2, CInteractionBox b1, CInteractionBox b2)
{
    Vec2 aSize = b1.size;
    Vec2 bSize = b2.size;
    Vec2 aPos = t1.pos - b1.halfSize;
    Vec2 bPos = t2.pos - b2.halfSize;

    bool x_overlap = (aPos.x + aSize.x > bPos.x) && (bPos.x + bSize.x > aPos.x);
    bool y_overlap = (aPos.y + aSize.y > bPos.y) && (bPos.y + bSize.y > aPos.y);

    return (x_overlap && y_overlap);
}

bool BaseCollisionManager::isCollided(CTransform t1, CTransform t2, CBox b1, CBox b2)
{
    Vec2 aSize = b1.size;
    Vec2 bSize = b2.size;
    Vec2 aPos = t1.pos - b1.halfSize;
    Vec2 bPos = t2.pos - b2.halfSize;

    bool x_overlap = (aPos.x + aSize.x > bPos.x) && (bPos.x + bSize.x > aPos.x);
    bool y_overlap = (aPos.y + aSize.y > bPos.y) && (bPos.y + bSize.y > aPos.y);

    return (x_overlap && y_overlap);
}

void BaseCollisionManager::renderQuadtree(SDL_Renderer* renderer, int zoom, Vec2 center, Vec2 cameraPosition)
{
    SDL_Color color = {255, 0, 0, 255};
    m_quadRoot->renderBoundary(renderer, zoom, center, cameraPosition, color);
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

void CollisionManager::doCollisions(Vec2 treePos, Vec2 treeSize)
{
    newQuadtree<CCollisionBox>(treePos, treeSize);
    auto& collisionPool = m_ECS->getComponentPool<CCollisionBox>();
    auto& transformPool = m_ECS->getComponentPool<CTransform>();
    
    auto quadVector = m_quadRoot->createQuadtreeVector();
    
    // loopQuadEntityPairs<CCollisionBox>(quadVector);

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
                // if ( ((collisionB.layer & collisionA.mask) != collisionB.layer) | 
                //      ((collisionA.layer & collisionB.mask) != collisionA.layer) ){
                //     continue; // No collision layer match
                // }
                auto& transformA = transformPool.getComponent(entityIDA);
                auto& transformB = transformPool.getComponent(entityIDB);
                auto overlap = collisionOverlap(transformA, transformB, collisionA.size, collisionB.size);
                if ( !isCollided(transformA, transformB, collisionA, collisionB) )
                {
                    continue; // No collision detected
                }
                handleCollision(entityIDA, collisionA.layer, entityIDB, collisionB.layer, overlap);
            }
        }
    }
}

InteractionManager::InteractionManager(ECS* ecs, Scene* scene)
{
    m_ECS = ecs;
    m_scene = scene;
    // registerHandler(PLAYER_LAYER, OBSTACLE_LAYER, handlePlayerObstacleCollision);
    // registerHandler(PLAYER_LAYER, ENEMY_LAYER, handlePlayerEnemyCollision);
    // registerHandler(PLAYER_LAYER, FRIENDLY_LAYER, handlePlayerObstacleCollision);
    // registerHandler(FRIENDLY_LAYER, OBSTACLE_LAYER, handleFriendlyObstacleCollision);
    // registerHandler(ENEMY_LAYER, PROJECTILE_LAYER, handleEnemyProjectileCollision);
    // registerHandler(ENEMY_LAYER, ENEMY_LAYER, handleEnemyEnemyCollision);
    // registerHandler(ENEMY_LAYER, OBSTACLE_LAYER, handleEnemyObstacleCollision);
    // registerHandler(PROJECTILE_LAYER, OBSTACLE_LAYER, handleProjectileObstacleCollision);
}

void InteractionManager::doInteractions(Vec2 treePos, Vec2 treeSize)
{
    auto& interactionPool = m_ECS->getComponentPool<CInteractionBox>();
    auto& transformPool = m_ECS->getComponentPool<CTransform>();
    auto& scriptPool = m_ECS->getComponentPool<CScript>();

    newQuadtree<CInteractionBox>(treePos, treeSize);
    auto quadVector = m_quadRoot->createQuadtreeVector();

    for (auto quadleaf : quadVector)
    {
        std::vector<Entity> entityVector = quadleaf->getObjects();

        for (size_t a = 0; a < entityVector.size(); ++a) {
            EntityID entityIDA = entityVector[a].getID();
            auto& interactionA = interactionPool.getComponent(entityIDA);
            auto& transformA = transformPool.getComponent(entityIDA);
            auto& scriptA = scriptPool.getComponent(entityIDA);

            for (size_t b = a + 1; b < entityVector.size(); ++b) {
                EntityID entityIDB = entityVector[b].getID();
                if ( entityIDA == entityIDB ) {
                    continue; // Skip self-collision
                }
                auto& interactionB = interactionPool.getComponent(entityIDB);
                if ( ((interactionB.layer & interactionA.mask) != interactionB.layer) |
                     ((interactionA.layer & interactionB.mask) != interactionA.layer) )
                {
                    continue; // No interaction layer match
                }
                auto& transformB = transformPool.getComponent(entityIDB);
                auto& scriptB = scriptPool.getComponent(entityIDB);
                if ( !isCollided(transformA, transformB, interactionA, interactionB) )
                {
                    continue; // No collision detected
                }
                scriptA.Instance->OnInteractFunction(entityIDB, interactionB.layer);
                scriptB.Instance->OnInteractFunction(entityIDA, interactionA.layer);                
            }
        }
    }
}

// template <typename T>
// void BaseCollisionManager::loopQuadEntityPairs(std::vector<std::shared_ptr<Quadtree>> quadVector)
// {
//     ComponentPool<T> boxPool = m_ECS->getComponentPool<T>();
//     ComponentPool<CTransform> transformPool = m_ECS->getComponentPool<CTransform>();
//     ComponentPool<CScript> scriptPool = m_ECS->getComponentPool<CScript>();

//     boxPool = static_cast<ComponentPool<T>>( new ComponentPool<CBox>() );
//     for (std::shared_ptr<Quadtree> quadleaf : quadVector)
//     {
//         std::vector<Entity> entityVector = quadleaf->getObjects();
//         loopEntityPairs(entityVector, boxPool, transformPool, scriptPool);
//     }
// }

// void BaseCollisionManager::loopEntityPairs( std::vector<Entity> entityVector, 
//                                             ComponentPool<CBox> boxPool,    
//                                             ComponentPool<CTransform> transformPool,
//                                             ComponentPool<CScript> scriptPool)
// {
//     for (size_t a = 0; a < entityVector.size(); ++a) 
//     {
//         EntityID entityIDA = entityVector[a].getID();
//         for (size_t b = a + 1; b < entityVector.size(); ++b) 
//         {
//             EntityID entityIDB = entityVector[b].getID();
//             if ( !layerCollision(entityIDA, entityIDB, boxPool, transformPool) )
//             {
//                 continue;
//             }

//         }
//     }
// }

// bool BaseCollisionManager::layerCollision(EntityID entityIDA, EntityID entityIDB, ComponentPool<CBox> boxPool, ComponentPool<CTransform> transformPool)
// {
//     if ( entityIDA == entityIDB ) {
//         return false; // Skip self-collision
//     }
//     auto& boxA = boxPool.getComponent(entityIDA);
//     auto& boxB = boxPool.getComponent(entityIDB);
//     if ( ((boxB.layer & boxA.mask) != boxB.layer) | ((boxB.layer & boxB.mask) != boxB.layer) ){
//         return false; // No interaction layer match
//     }
//     auto& transformA = transformPool.getComponent(entityIDA);
//     auto& transformB = transformPool.getComponent(entityIDB);
//     if ( !isCollided(transformA, transformB, boxA, boxB) )
//     {
//         return false; // No collision detected
//     }
//     return true;
// }