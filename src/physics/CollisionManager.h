#pragma once
#include <functional>
#include <array>

#include "ecs/Components.h"
#include "ecs/ECS.hpp"
#include "physics/Quadtree.h"
// #include "scenes/Scene_Play.h"
class Scene_Play;

using Handler = std::function<void(Entity, Entity, Vec2)>;
using CollisionMatrix = std::array<std::array<Handler, MAX_LAYERS>, MAX_LAYERS>;

class BaseCollisionManager
{
    private:
    
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
    void renderQuadtree(SDL_Renderer*renderer, int zoom, Vec2 screenCenter, Vec2 camPos);
};
class CollisionManager : public BaseCollisionManager
{    
    public:
    CollisionManager(ECS* ecs, Scene_Play* scene);
    void doCollisions(Vec2 pos, Vec2 size);
    
};

class InteractionManager : public BaseCollisionManager
{
    bool talkToNPC(Entity player, Entity friendly);
    bool possesNPC(Entity player, Entity friendly);
    void handlePlayerEnemy(Entity player, Entity enemy, Vec2 overlap);
    void handlePlayerFriendly(Entity player, Entity friendly, Vec2 overlap);
    void handlePlayerLoot(Entity player, Entity loot, Vec2 overlap);
    
    public:
    InteractionManager(ECS* ecs, Scene_Play* scene);
    void doInteractions(Vec2 pos, Vec2 size);
};

