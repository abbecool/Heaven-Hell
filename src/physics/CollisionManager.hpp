#pragma once
#include <functional>
#include <array>

#include "ecs/Components.hpp"
#include "ecs/ECS.hpp"
#include "physics/Quadtree.hpp"
#include "render/RenderBackend.hpp"
// #include "scenes/Scene_Play.hpp"
class Scene_Play;

using Handler = std::function<void(Entity, Entity, Vec2)>;
using CollisionMatrix = std::array<std::array<Handler, MAX_LAYERS>, MAX_LAYERS>;

class BaseCollisionManager
{
    private:
    // Helper functions for collision calculation
    Vec2 calculateDelta(Vec2 aPos, Vec2 aSize, Vec2 bPos, Vec2 bSize) const;
    Vec2 calculateHorizontalMovement(const Vec2& aPos, const Vec2& aSize, const Vec2& bPos, const Vec2& bSize, const Vec2& overlap, const Vec2& prevOverlap) const;
    Vec2 calculateVerticalMovement(const Vec2& aPos, const Vec2& aSize, const Vec2& bPos, const Vec2& bSize, const Vec2& overlap, const Vec2& prevOverlap) const;
    
    public:
    CollisionMatrix m_handlerMatrix;
    ECS* m_ECS;
    Scene_Play* m_scene;
    
    std::unique_ptr<Quadtree> m_quadRoot;
    
    BaseCollisionManager(){};

    void registerHandler(
        CollisionMask layerA,
        CollisionMask layerB, 
        Handler handler
    );
    void handleCollision(
        EntityID entityA, 
        CollisionMask layerA, 
        EntityID entityB, 
        CollisionMask layerB, 
        Vec2 overlap
    );
    Vec2 collisionOverlap(CTransform t1, CTransform t2, Vec2 box1, Vec2 box2);
    bool isCollided(CTransform t1, CTransform t2, CCollisionBox b1, CCollisionBox b2);
    bool isCollided(CTransform t1, CTransform t2, CInteractionBox b1, CInteractionBox b2);
    bool isCollided(CTransform t1, CTransform t2, CBox b1, CBox b2);
    
    template <typename T>
    void newQuadtree(Vec2 pos, Vec2 size);
    void renderQuadtree(RenderBackend& renderer, int zoom, Vec2 screenCenter, Vec2 camPos);
};

class CollisionManager : public BaseCollisionManager
{    
    private:
    // Helper functions for collision processing
    void processQuadtreeLeaf(std::vector<Entity>& entities);
    
    public:
    CollisionManager(ECS* ecs, Scene_Play* scene);
    void doCollisions(Vec2 pos, Vec2 size);
    
};

class InteractionManager : public BaseCollisionManager
{
    private:
    // Helper functions for interaction processing
    bool checkInteractionLayerMask(const CInteractionBox& boxA, const CInteractionBox& boxB) const;
    void processInteractionLeaf(std::vector<Entity>& entities);
    
    bool talkToNPC(Entity player, Entity friendly);
    bool possesNPC(Entity player, Entity friendly);
    void handlePlayerEnemy(Entity player, Entity enemy, Vec2 overlap);
    void handlePlayerFriendly(Entity player, Entity friendly, Vec2 overlap);
    void handlePlayerLoot(Entity player, Entity loot, Vec2 overlap);
    void handlePlayerArea(Entity player, Entity area, Vec2 overlap);
    
    public:
    InteractionManager(ECS* ecs, Scene_Play* scene);
    void doInteractions(Vec2 pos, Vec2 size);
};

