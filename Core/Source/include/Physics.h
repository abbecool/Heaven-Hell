#pragma once

#include <memory>

#include "Quadtree.hpp"
#include "Entity.h"
#include "Vec2.h"

class Physics
{   
    std::unique_ptr<Quadtree> m_quadroot;
    public:
        bool isCollided(CTransform t1, CBoundingBox b1, CTransform t2, CBoundingBox b2);
        bool isStandingIn(Entity entity1, Entity entity2);
        Vec2 overlap(CTransform t1, CBoundingBox b1, CTransform t2, CBoundingBox b2);
        Vec2 calculateOverlap(CTransform t1, CBoundingBox b1, CTransform t2, CBoundingBox b2);
        Vec2 knockback(CKnockback& knockback);
        void clearQuadtree();
        std::unique_ptr<Quadtree> createQuadtree();

};