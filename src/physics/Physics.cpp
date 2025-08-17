#include "physics/Physics.h"
#include "ecs/Entity.h"
#include "ecs/ECS.hpp"
#include "ecs/Components.h"
#include <cstdlib>
#include <memory>

bool Physics::PointInRect(const Vec2& point, const Vec2& rectPos, const Vec2& rectSize){
    return (point.x >= rectPos.x - rectSize.x/2 && point.x <= rectPos.x + rectSize.x/2 &&
            point.y >= rectPos.y - rectSize.y/2 && point.y <= rectPos.y + rectSize.y/2);
}

Vec2 Physics::knockback(CKnockback& knockback){
    knockback.timeElapsed += 16;
    if (knockback.timeElapsed < knockback.duration) {
        return knockback.direction.norm(knockback.magnitude)*16/knockback.duration;
    } else {
        // Reset the  when the duration is over
        knockback.duration = 0;
        return Vec2{0, 0};
    }
}

void Physics::clearQuadtree()
{
    m_quadRoot = nullptr;
}

void Physics::createQuadtree(Vec2 pos, Vec2 size)
{
    m_quadRoot = std::make_unique<Quadtree>(pos, size);
}

void Physics::insertQuadtree(Entity e)
{
    m_quadRoot->insert<CCollisionBox>(e);
}

void Physics::renderQuadtree(SDL_Renderer* renderer, int zoom, Vec2 screenCenter, Vec2 camPos)
{
    m_quadRoot->renderBoundary(renderer, zoom, screenCenter, camPos, {255, 0, 0, 255});
}

int Physics::countQuadtree(int count)
{
    return m_quadRoot->countLeafs(count);
}

std::vector<std::shared_ptr<Quadtree>> Physics::createQuadtreeVector()
{
    std::vector<std::shared_ptr<Quadtree>> quadtreeVector = m_quadRoot->createQuadtreeVector();
    return quadtreeVector;
}

void Physics::clearInteractionQuadtree()
{
    m_interactionQuadRoot = nullptr;
}

void Physics::createInteractionQuadtree(Vec2 pos, Vec2 size)
{
    m_interactionQuadRoot = std::make_unique<Quadtree>(pos, size);
}

void Physics::insertInteractionQuadtree(Entity e)
{
    m_interactionQuadRoot->insert<CInteractionBox>(e);
}

void Physics::renderInteractionQuadtree(SDL_Renderer* renderer, int zoom, Vec2 screenCenter, Vec2 camPos)
{
    m_interactionQuadRoot->renderBoundary(renderer, zoom, screenCenter, camPos, {0, 0, 255, 255});
}

int Physics::countInteractionQuadtree(int count)
{
    return m_interactionQuadRoot->countLeafs(count);
}

std::vector<std::shared_ptr<Quadtree>> Physics::createInteractionQuadtreeVector()
{
    std::vector<std::shared_ptr<Quadtree>> quadtreeVector = m_interactionQuadRoot->createQuadtreeVector();
    return quadtreeVector;
}

