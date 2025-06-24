#pragma once
#include <functional>
#include <array>

#include "Components.h"
#include "ECS.hpp"
#include "Quadtree.h"
#include "Scene.h"

using CollisionHandler = std::function<void(Entity, Entity, Vec2)>;
using CollisionMatrix = std::array<std::array<CollisionHandler, MAX_LAYERS>, MAX_LAYERS>;

class CollisionManager
{
    private:
    CollisionMatrix m_handlerMatrix;
    ECS* m_ECS;
    Scene* m_scene;
    
    std::unique_ptr<Quadtree> m_quadRoot;
    std::unique_ptr<Quadtree> m_interactionQuadRoot;

    void registerHandler(CollisionMask layerA, CollisionMask layerB, CollisionHandler handler);

    void handleCollision(EntityID entityA, CollisionMask layerA, EntityID entityB, CollisionMask layerB, Vec2 overlap);
    void createQuadtree(Vec2 treePos, Vec2 treeSize);
    
    Vec2 collisionOverlap(CTransform t1, CTransform t2, Vec2 box1, Vec2 box2);
    bool isCollided(CTransform t1, CTransform t2, CCollisionBox b1, CCollisionBox b2);

    public:
    CollisionManager(ECS* ecs, Scene* scene);
    void newQuadtree(Vec2 pos, Vec2 size);
    void newInteractionQuadtree(Vec2 pos, Vec2 size);
    void doCollisions();
    void renderQuadtree(SDL_Renderer*renderer, int zoom, Vec2 screenCenter, Vec2 camPos);
    void renderInteractionQuadtree(SDL_Renderer*renderer, int zoom, Vec2 screenCenter, Vec2 camPos);

};