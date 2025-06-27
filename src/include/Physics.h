#pragma once

#include <memory>

#include "Quadtree.h"
#include "Entity.h"
#include "Vec2.h"

class Physics
{   
    public:
        std::shared_ptr<Quadtree> m_quadRoot;
        std::shared_ptr<Quadtree> m_interactionQuadRoot;
        bool isCollided(CTransform t1, CCollisionBox b1, CTransform t2, CCollisionBox b2);
        bool isCollided(CTransform t1, CInteractionBox b1, CTransform t2, CInteractionBox b2);
        bool isStandingIn(Entity entity1, Entity entity2);
        Vec2 overlap(CTransform t1, CCollisionBox b1, CTransform t2, CCollisionBox b2);
        Vec2 overlap(CTransform t1, CInteractionBox b1, CTransform t2, CInteractionBox b2);
        Vec2 calculateOverlap(CTransform t1, CCollisionBox b1, CTransform t2, CCollisionBox b2);
        Vec2 knockback(CKnockback& knockback);

        void clearQuadtree();
        void createQuadtree(Vec2 pos, Vec2 size);
        void insertQuadtree(Entity e);
        void renderQuadtree(SDL_Renderer*renderer, int zoom, Vec2 screenCenter, Vec2 camPos);
        int countQuadtree(int count);
        std::vector<std::shared_ptr<Quadtree>> createQuadtreeVector();

        void clearInteractionQuadtree();
        void createInteractionQuadtree(Vec2 pos, Vec2 size);
        void insertInteractionQuadtree(Entity e);
        void renderInteractionQuadtree(SDL_Renderer*renderer, int zoom, Vec2 screenCenter, Vec2 camPos);
        int countInteractionQuadtree(int count);
        std::vector<std::shared_ptr<Quadtree>> createInteractionQuadtreeVector();
};