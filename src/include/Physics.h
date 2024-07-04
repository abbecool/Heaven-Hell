#pragma once

#include <memory>
#include "Entity.h"
#include "Vec2.h"

class Physics
{
    public:
        bool isCollided(std::shared_ptr<Entity> entity1, std::shared_ptr<Entity> entity2);
        bool isStandingIn(std::shared_ptr<Entity> entity1, std::shared_ptr<Entity> entity2);
        Vec2 Overlap(std::shared_ptr<Entity> p, std::shared_ptr<Entity> o);
};