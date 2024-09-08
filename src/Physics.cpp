#include "Physics.h"
#include "Entity.h"
#include "Components.h"
#include <cstdlib>
#include <memory>

bool Physics::isCollided(std::shared_ptr<Entity> a, std::shared_ptr<Entity> b)
{
    if (a->id() == b->id())
    {
        return false;
    }

    Vec2 aSize = a->getComponent<CBoundingBox>().size;
    Vec2 bSize = b->getComponent<CBoundingBox>().size;
    Vec2 aPos = a->getComponent<CTransform>().pos - a->getComponent<CBoundingBox>().halfSize;
    Vec2 bPos = b->getComponent<CTransform>().pos - b->getComponent<CBoundingBox>().halfSize;

    bool x_overlap = (aPos.x + aSize.x > bPos.x) && (bPos.x + bSize.x > aPos.x);
    bool y_overlap = (aPos.y + aSize.y > bPos.y) && (bPos.y + bSize.y > aPos.y);

    return (x_overlap && y_overlap);
}

bool Physics::isStandingIn(std::shared_ptr<Entity> a, std::shared_ptr<Entity> b)
{
    if (a->id() == b->id())
    {
        return false;
    }
    
    Vec2 aSize = a->getComponent<CBoundingBox>().size;
    Vec2 aHalfSize = a->getComponent<CBoundingBox>().halfSize;
    Vec2 bSize = b->getComponent<CBoundingBox>().size;
    Vec2 aPos = a->getComponent<CTransform>().pos - a->getComponent<CBoundingBox>().halfSize;
    Vec2 bPos = b->getComponent<CTransform>().pos - b->getComponent<CBoundingBox>().halfSize;

    bool x_overlap = (aPos.x + aHalfSize.x > bPos.x) && (bPos.x + bSize.x > aPos.x + aHalfSize.x);
    // bool x_overlap = (aPos.x + aSize.x > bPos.x) && (bPos.x + bSize.x > aPos.x);
    bool y_overlap = (aPos.y + aSize.y <= bPos.y + bSize.y) && (aPos.y + aSize.y > bPos.y);

    return (x_overlap && y_overlap);
}

Vec2 Physics::overlap(std::shared_ptr<Entity> a, std::shared_ptr<Entity> b)
{    
    Vec2 aSize = a->getComponent<CBoundingBox>().halfSize;
    Vec2 bSize = b->getComponent<CBoundingBox>().halfSize;
    Vec2 aPos = a->getComponent<CTransform>().pos - a->getComponent<CBoundingBox>().halfSize;
    Vec2 bPos = b->getComponent<CTransform>().pos - b->getComponent<CBoundingBox>().halfSize;
    Vec2 aPrevPos = a->getComponent<CTransform>().prevPos - a->getComponent<CBoundingBox>().halfSize;
    Vec2 bPrevPos = b->getComponent<CTransform>().prevPos - b->getComponent<CBoundingBox>().halfSize;

    Vec2 delta          =   ( (aPos       + aSize) - (bPos      + bSize) ).abs_elem();
    Vec2 prevDelta      =   ( (aPrevPos   + aSize) - (bPrevPos  + bSize) ).abs_elem();
    Vec2 overlap        =   aSize + bSize - delta;
    Vec2 prevOverlap    =   aSize + bSize - prevDelta;

    Vec2 move = { 0, 0 };
    if (prevOverlap.y > 0)
    {
        if ((aPos.x + aSize.x) > (bPos.x + bSize.x))
        {
            move += Vec2 { overlap.x, 0 };
        }
        if ((aPos.x + aSize.x) < (bPos.x + bSize.x))
        {
            move -= Vec2 { overlap.x, 0 };
        }
    }

    if (prevOverlap.x > 0)
    {
        if ((aPos.y + aSize.y/2) > (bPos.y + bSize.y/2))
        {
            move +=  Vec2 { 0, overlap.y };
        }
        if ((aPos.y + aSize.y/2) < (bPos.y + bSize.y/2))
        {
            move -=  Vec2 { 0, overlap.y };
        }
    }

    return move;
}

Vec2 Physics::getOverlap(std::shared_ptr<Entity> a, std::shared_ptr<Entity> b) {
    // todo: return the overlap rectangle size of the bouding boxes of enetity a and b
    Vec2 posA = a->getComponent<CTransform>().pos;
    Vec2 sizeA = a->getComponent<CBoundingBox>().halfSize;
    Vec2 posB = b->getComponent<CTransform>().pos;
    Vec2 sizeB = b->getComponent<CBoundingBox>().halfSize;
    Vec2 delta{ std::abs(posA.x - posB.x), std::abs(posA.y - posB.y) };
    float ox = sizeA.x + sizeB.x - delta.x;
    float oy = sizeA.y + sizeB.y - delta.y;
    return Vec2(ox, oy);
}

Vec2 Physics::knockback(CKnockback& knockback){
    knockback.timeElapsed += 16;
    if (knockback.timeElapsed < knockback.duration) {
        return knockback.direction.norm()*knockback.magnitude/(knockback.duration/16);
    } else {
        // Reset the  when the duration is over
        knockback.duration = 0;
        return Vec2{0, 0};
    }
}