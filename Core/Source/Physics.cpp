#include "Physics.h"
#include "Entity.h"
#include "ECS.hpp"
#include "Components.h"
#include <cstdlib>
#include <memory>

bool Physics::isCollided(CTransform t1, CCollisionBox b1, CTransform t2, CCollisionBox b2)
{
    Vec2 aSize = b1.size;
    Vec2 bSize = b2.size;
    Vec2 aPos = t1.pos - b1.halfSize;
    Vec2 bPos = t2.pos - b2.halfSize;

    bool x_overlap = (aPos.x + aSize.x > bPos.x) && (bPos.x + bSize.x > aPos.x);
    bool y_overlap = (aPos.y + aSize.y > bPos.y) && (bPos.y + bSize.y > aPos.y);

    return (x_overlap && y_overlap);
}

bool Physics::isStandingIn(Entity a, Entity b)
{
    if (a.getID() == b.getID())
    {
        return false;
    }
    
    Vec2 aSize = a.getComponent<CCollisionBox>().size;
    Vec2 aHalfSize = a.getComponent<CCollisionBox>().halfSize;
    Vec2 bSize = b.getComponent<CCollisionBox>().size;
    Vec2 aPos = a.getComponent<CTransform>().pos - a.getComponent<CCollisionBox>().halfSize;
    Vec2 bPos = b.getComponent<CTransform>().pos - b.getComponent<CCollisionBox>().halfSize;
    
    bool x_overlap = (aPos.x + aHalfSize.x > bPos.x) && (bPos.x + bSize.x > aPos.x + aHalfSize.x);
    bool y_overlap = (aPos.y + aSize.y <= bPos.y + bSize.y) && (aPos.y + aSize.y > bPos.y);
    
    return (x_overlap && y_overlap);
}

Vec2 Physics::overlap(CTransform t1, CCollisionBox b1, CTransform t2, CCollisionBox b2)
{    
    Vec2 aSize = b1.halfSize;
    Vec2 bSize = b2.halfSize;
    Vec2 aPos = t1.pos - b1.halfSize;
    Vec2 bPos = t2.pos - b2.halfSize;
    Vec2 aPrevPos = t1.prevPos - b1.halfSize;
    Vec2 bPrevPos = t2.prevPos - b2.halfSize;
    
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

Vec2 Physics::calculateOverlap(CTransform t1, CCollisionBox b1, CTransform t2, CCollisionBox b2) {
    // todo: return the overlap rectangle size of the bouding boxes of enetity a and b
    Vec2 posA = t1.pos;
    Vec2 sizeA = b1.halfSize;
    Vec2 posB = t2.pos;
    Vec2 sizeB = b2.halfSize;
    Vec2 delta{ std::abs(posA.x - posB.x), std::abs(posA.y - posB.y) };
    float ox = sizeA.x + sizeB.x - delta.x;
    float oy = sizeA.y + sizeB.y - delta.y;
    return Vec2(ox, oy);
}

Vec2 Physics::knockback(CKnockback& knockback){
    knockback.timeElapsed += 16;
    if (knockback.timeElapsed < knockback.duration) {
        return knockback.direction.norm( (float)knockback.magnitude ) / (float)(knockback.duration/16);
    } else {
        // Reset the  when the duration is over
        knockback.duration = 0;
        return Vec2{0, 0};
    }
}

void Physics::clearQuadtree()
{
    m_quadroot = nullptr;
}

void Physics::createQuadtree(Vec2 pos, Vec2 size)
{
    m_quadroot = std::make_unique<Quadtree>(pos.x, pos.y, size.x, size.y);
}

void Physics::insertQuadtree(Entity e)
{
    m_quadroot->insert(e);
}

void Physics::renderQuadtree(SDL_Renderer* renderer, int zoom, Vec2 screenCenter, Vec2 camPos)
{
    m_quadroot->renderBoundary(renderer, zoom, screenCenter, camPos);
}

int Physics::countQuadtree(int count)
{
    return m_quadroot->countLeafs(count);
}

std::vector<std::shared_ptr<Quadtree>> Physics::createQuadtreeVector()
{
    std::vector<std::shared_ptr<Quadtree>> quadtreeVector = m_quadroot->createQuadtreeVector();
    return quadtreeVector;
}
