#include "Physics.h"
#include "Entity.h"
#include <cstdlib>
#include <memory>

bool Physics::isCollided(std::shared_ptr<Entity> entity1, std::shared_ptr<Entity> entity2)
{
    if (entity1->id() == entity2->id())
    {
        return false;
    }

    bool x_overlap =    (entity1->getComponent<CTransform>().pos.x + entity1->getComponent<CShape>().size.x > entity2->getComponent<CTransform>().pos.x) && 
                        (entity2->getComponent<CTransform>().pos.x + entity2->getComponent<CShape>().size.x > entity1->getComponent<CTransform>().pos.x);
    bool y_overlap =    (entity1->getComponent<CTransform>().pos.y + entity1->getComponent<CShape>().size.y > entity2->getComponent<CTransform>().pos.y) && 
                        (entity2->getComponent<CTransform>().pos.y + entity2->getComponent<CShape>().size.y > entity1->getComponent<CTransform>().pos.y);

    return (x_overlap && y_overlap);
}

bool Physics::isStandingIn(std::shared_ptr<Entity> entity1, std::shared_ptr<Entity> entity2)
{
    if (entity1->id() == entity2->id())
    {
        return false;
    }

    bool x_center_overlap = (entity1->getComponent<CTransform>().pos.x + entity1->getComponent<CShape>().size.x/2 > entity2->getComponent<CTransform>().pos.x) && 
                            (entity2->getComponent<CTransform>().pos.x + entity2->getComponent<CShape>().size.x/2 > entity1->getComponent<CTransform>().pos.x);
    bool y_overlap =    (entity1->getComponent<CTransform>().pos.y + entity1->getComponent<CShape>().size.y <= entity2->getComponent<CTransform>().pos.y + entity2->getComponent<CShape>().size.y) &&
                        (entity1->getComponent<CTransform>().pos.y + entity1->getComponent<CShape>().size.y > entity2->getComponent<CTransform>().pos.y);

    return (x_center_overlap && y_overlap);
}

Vec2 Physics::Overlap(std::shared_ptr<Entity> p, std::shared_ptr<Entity> o)
{
    Vec2 delta          =   ( (p->getComponent<CTransform>().pos       + p->getComponent<CShape>().size/2) - (o->getComponent<CTransform>().pos      + o->getComponent<CShape>().size/2) ).abs_elem();
    Vec2 prevDelta      =   ( (p->getComponent<CTransform>().prevPos   + p->getComponent<CShape>().size/2) - (o->getComponent<CTransform>().prevPos  + o->getComponent<CShape>().size/2) ).abs_elem();
    Vec2 overlap        =   p->getComponent<CShape>().size/2 + o->getComponent<CShape>().size/2 - delta;
    Vec2 prevOverlap    =   p->getComponent<CShape>().size/2 + o->getComponent<CShape>().size/2 - prevDelta;

    Vec2 move = { 0, 0 };
    if (prevOverlap.y > 0)
    {
        if ((p->getComponent<CTransform>().pos.x + p->getComponent<CShape>().size.x/2) > (o->getComponent<CTransform>().pos.x + o->getComponent<CShape>().size.x/2))
        {
            move += Vec2 { overlap.x, 0 };
        }
        if ((p->getComponent<CTransform>().pos.x + p->getComponent<CShape>().size.x/2) < (o->getComponent<CTransform>().pos.x + o->getComponent<CShape>().size.x/2))
        {
            move -= Vec2 { overlap.x, 0 };
        }
    }

    if (prevOverlap.x > 0)
    {
        if ((p->getComponent<CTransform>().pos.y + p->getComponent<CShape>().size.y/2) > (o->getComponent<CTransform>().pos.y + o->getComponent<CShape>().size.y/2))
        {
            move +=  Vec2 { 0, overlap.y };
        }
        if ((p->getComponent<CTransform>().pos.y + p->getComponent<CShape>().size.y/2) < (o->getComponent<CTransform>().pos.y + o->getComponent<CShape>().size.y/2))
        {
            move -=  Vec2 { 0, overlap.y };
        }
    }

    // Vec2 posA = p->getComponent<CTransform>().pos;
    // Vec2 sizeA = p->getComponent<CShape>().size/2;
    // Vec2 posB = o->getComponent<CTransform>().pos;
    // Vec2 sizeB = o->getComponent<CShape>().size/2;
    // Vec2 delta{ std::abs(posA.x - posB.x), std::abs(posA.y - posB.y) };
    // float ox = sizeA.x + sizeB.x - delta.x;
    // float oy = sizeA.y + sizeB.y - delta.y;
    // return Vec2(ox, oy);

    return move;
}