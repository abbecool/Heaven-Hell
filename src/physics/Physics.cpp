#include "physics/Physics.hpp"
#include "ecs/Entity.hpp"
#include "ecs/ECS.hpp"
#include "ecs/Components.hpp"
#include <cstdlib>
#include <memory>

bool Physics::PointInRect(const Vec2& point, const Vec2& rectPos, const Vec2& rectSize){
    return (point.x >= rectPos.x - rectSize.x/2 && point.x <= rectPos.x + rectSize.x/2 &&
            point.y >= rectPos.y - rectSize.y/2 && point.y <= rectPos.y + rectSize.y/2);
}

bool Physics::PointInCollider(const Vec2& point, const CTransform& transform, const CCollider& collider, bool includeTriggers)
{
    for (const auto& shape : collider.shapes) {
        if (!includeTriggers && shape.isTrigger) {
            continue;
        }
        if (PointInRect(point, transform.pos + shape.offset, shape.size)) {
            return true;
        }
    }
    return false;
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
    if (!e.hasComponent<CCollider>() || !e.hasComponent<CTransform>()) {
        return;
    }

    const auto& collider = e.getComponent<CCollider>();
    const auto& transform = e.getComponent<CTransform>();
    for (size_t i = 0; i < collider.shapes.size(); ++i) {
        const auto& shape = collider.shapes[i];
        m_quadRoot->insert(i, transform.pos + shape.offset, shape.size);
    }
}

void Physics::renderQuadtree(RenderBackend& renderer)
{
    m_quadRoot->renderBoundary(renderer, {255, 0, 0, 255});
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
    if (!e.hasComponent<CCollider>() || !e.hasComponent<CTransform>()) {
        return;
    }

    const auto& collider = e.getComponent<CCollider>();
    const auto& transform = e.getComponent<CTransform>();
    for (size_t i = 0; i < collider.shapes.size(); ++i) {
        const auto& shape = collider.shapes[i];
        if (shape.isTrigger) {
            m_interactionQuadRoot->insert(i, transform.pos + shape.offset, shape.size);
        }
    }
}

void Physics::renderInteractionQuadtree(RenderBackend& renderer)
{
    m_interactionQuadRoot->renderBoundary(renderer, {0, 0, 255, 255});
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

Vec2 Physics::aStar(Vec2 start, Vec2 target){
    Vec2 velocity = {0,0};
    return velocity;
}
