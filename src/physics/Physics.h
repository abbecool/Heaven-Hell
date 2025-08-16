#pragma once

#include <memory>

#include "physics/Quadtree.h"
#include "ecs/Entity.h"
#include "physics/Vec2.h"

class Physics
{   
    public:
        std::shared_ptr<Quadtree> m_quadRoot;
        std::shared_ptr<Quadtree> m_interactionQuadRoot;
        Vec2 knockback(CKnockback& knockback);
        bool PointInRect(const Vec2& point, const Vec2& rectPos, const Vec2& rectSize);

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