#pragma once

#include <memory>

#include "physics/Quadtree.hpp"
#include "ecs/Entity.hpp"
#include "physics/Vec2.hpp"
#include "render/RenderBackend.hpp"

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
        void renderQuadtree(RenderBackend& renderer);
        int countQuadtree(int count);
        std::vector<std::shared_ptr<Quadtree>> createQuadtreeVector();

        void clearInteractionQuadtree();
        void createInteractionQuadtree(Vec2 pos, Vec2 size);
        void insertInteractionQuadtree(Entity e);
        void renderInteractionQuadtree(RenderBackend& renderer);
        int countInteractionQuadtree(int count);
        std::vector<std::shared_ptr<Quadtree>> createInteractionQuadtreeVector();
        
        Vec2 aStar(Vec2 pos, Vec2 targetPos);
};
