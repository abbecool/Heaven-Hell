#pragma once

#include <memory>
#include "Entity.h"
#include "Vec2.h"

class Physics
{
    public:
        bool isCollided(std::shared_ptr<Entity> entity1, std::shared_ptr<Entity> entity2);
        bool isStandingIn(std::shared_ptr<Entity> entity1, std::shared_ptr<Entity> entity2);
        Vec2 overlap(std::shared_ptr<Entity> p, std::shared_ptr<Entity> o);
        Vec2 getOverlap(std::shared_ptr<Entity> a, std::shared_ptr<Entity> b);
        Vec2 knockback(CKnockback& knockback);

};