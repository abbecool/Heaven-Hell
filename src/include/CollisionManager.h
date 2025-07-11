#pragma once
#include <functional>
#include <array>

#include "Components.h"
#include "ECS.hpp"
#include "Quadtree.h"
#include "Scene.h"

using Handler = std::function<void(Entity, Entity, Vec2)>;
using CollisionMatrix = std::array<std::array<Handler, MAX_LAYERS>, MAX_LAYERS>;

class BaseCollisionManager
{
    private:
    
    public:
    CollisionMatrix m_handlerMatrix;
    ECS* m_ECS;
    Scene* m_scene;
    
    std::unique_ptr<Quadtree> m_quadRoot;
    
    BaseCollisionManager(){};

    void registerHandler(CollisionMask layerA, CollisionMask layerB, Handler handler);
    void handleCollision(EntityID entityA, CollisionMask layerA, EntityID entityB, CollisionMask layerB, Vec2 overlap);
    Vec2 collisionOverlap(CTransform t1, CTransform t2, Vec2 box1, Vec2 box2);
    bool isCollided(CTransform t1, CTransform t2, CCollisionBox b1, CCollisionBox b2);
    bool isCollided(CTransform t1, CTransform t2, CInteractionBox b1, CInteractionBox b2);
    bool isCollided(CTransform t1, CTransform t2, CBox b1, CBox b2);

    template <typename T>
    void loopQuadEntityPairs(std::vector<std::shared_ptr<Quadtree>> quadVector);
    void loopQuadEntityPairs(std::vector<Entity> entityVector);
    bool layerCollision(EntityID entityIDA, EntityID entityIDB, ComponentPool<CBox> boxPool, ComponentPool<CTransform> transformPool);
    void loopEntityPairs(   std::vector<Entity> entityVector, 
                            ComponentPool<CBox> hitBoxPool, 
                            ComponentPool<CTransform> transformPool,
                            ComponentPool<CScript> scriptPool);
    
    template <typename T>
    void newQuadtree(Vec2 pos, Vec2 size);
    void renderQuadtree(SDL_Renderer*renderer, int zoom, Vec2 screenCenter, Vec2 camPos);
};
class CollisionManager : public BaseCollisionManager
{
    private:
    // CollisionMatrix m_handlerMatrix;
    // ECS* m_ECS;
    // Scene* m_scene;
    
    // std::unique_ptr<Quadtree> m_interactionQuadRoot;

    // void registerHandler(CollisionMask layerA, CollisionMask layerB, Handler handler);
    // void handleCollision(EntityID entityA, CollisionMask layerA, EntityID entityB, CollisionMask layerB, Vec2 overlap);
    // Vec2 collisionOverlap(CTransform t1, CTransform t2, Vec2 box1, Vec2 box2);
    // bool isCollided(CTransform t1, CTransform t2, CCollisionBox b1, CCollisionBox b2);
    
    public:
    CollisionManager(ECS* ecs, Scene* scene);
    // void newQuadtree(Vec2 pos, Vec2 size);
    // void newInteractionQuadtree(Vec2 pos, Vec2 size);
    void doCollisions(Vec2 pos, Vec2 size);
    // void renderQuadtree(SDL_Renderer*renderer, int zoom, Vec2 screenCenter, Vec2 camPos);
    // void renderInteractionQuadtree(SDL_Renderer*renderer, int zoom, Vec2 screenCenter, Vec2 camPos);
    
};

class InteractionManager : public BaseCollisionManager
{
    private:
    // void registerInteractionHandler(CollisionMask layerA, CollisionMask layerB, Handler handler);
    // void handleInteraction(EntityID entityA, CollisionMask layerA, EntityID entityB, CollisionMask layerB, Vec2 overlap);
    public:
    InteractionManager(ECS* ecs, Scene* scene);
    void doInteractions(Vec2 pos, Vec2 size);

};