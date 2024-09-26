#pragma once

#include <memory>
#include "Entity.h"
#include "Vec2.h"

class Physics
{
    public:
        bool isCollided(Entity entity1, Entity entity2);
        bool isStandingIn(Entity entity1, Entity entity2);
        Vec2 overlap(Entity p, Entity o);
        Vec2 getOverlap(Entity a, Entity b);
        Vec2 knockback(CKnockback& knockback);

};