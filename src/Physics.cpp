#include "Physics.h"
#include "Entity.h"
#include <cstdlib>
#include <memory>

bool Physics::isCollided(std::shared_ptr<Entity> entity1, std::shared_ptr<Entity> entity2)
{
    if (entity1->getId() == entity2->getId())
    {
        return false;
    }

    bool x_overlap =    (entity1->cTransform->pos.x + entity1->cShape->size.x > entity2->cTransform->pos.x) && 
                        (entity2->cTransform->pos.x + entity2->cShape->size.x > entity1->cTransform->pos.x);
    bool y_overlap =    (entity1->cTransform->pos.y + entity1->cShape->size.y > entity2->cTransform->pos.y) && 
                        (entity2->cTransform->pos.y + entity2->cShape->size.y > entity1->cTransform->pos.y);

    return (x_overlap && y_overlap);
}

bool Physics::isStandingIn(std::shared_ptr<Entity> entity1, std::shared_ptr<Entity> entity2)
{
    if (entity1->getId() == entity2->getId())
    {
        return false;
    }

    bool x_center_overlap = (entity1->cTransform->pos.x + entity1->cShape->size.x/2 > entity2->cTransform->pos.x) && 
                            (entity2->cTransform->pos.x + entity2->cShape->size.x/2 > entity1->cTransform->pos.x);
    bool y_overlap =    (entity1->cTransform->pos.y + entity1->cShape->size.y <= entity2->cTransform->pos.y + entity2->cShape->size.y) &&
                        (entity1->cTransform->pos.y + entity1->cShape->size.y > entity2->cTransform->pos.y);

    return (x_center_overlap && y_overlap);
}

Vec2 Physics::Overlap(std::shared_ptr<Entity> p, std::shared_ptr<Entity> o)
{
    Vec2 delta          =   ( (p->cTransform->pos       + p->cShape->size/2) - (o->cTransform->pos      + o->cShape->size/2) ).abs_elem();
    Vec2 prevDelta      =   ( (p->cTransform->prevPos   + p->cShape->size/2) - (o->cTransform->prevPos  + o->cShape->size/2) ).abs_elem();
    Vec2 overlap        =   p->cShape->size/2 + o->cShape->size/2 - delta;
    Vec2 prevOverlap    =   p->cShape->size/2 + o->cShape->size/2 - prevDelta;

    Vec2 move = { 0, 0 };
    if (prevOverlap.y > 0)
    {
        if ((p->cTransform->pos.x + p->cShape->size.x/2) > (o->cTransform->pos.x + o->cShape->size.x/2))
        {
            move += Vec2 { overlap.x, 0 };
        }
        if ((p->cTransform->pos.x + p->cShape->size.x/2) < (o->cTransform->pos.x + o->cShape->size.x/2))
        {
            move -= Vec2 { overlap.x, 0 };
        }
    }

    if (prevOverlap.x > 0)
    {
        if ((p->cTransform->pos.y + p->cShape->size.y/2) > (o->cTransform->pos.y + o->cShape->size.y/2))
        {
            move +=  Vec2 { 0, overlap.y };
        }
        if ((p->cTransform->pos.y + p->cShape->size.y/2) < (o->cTransform->pos.y + o->cShape->size.y/2))
        {
            move -=  Vec2 { 0, overlap.y };
        }
    }

    return move;
}