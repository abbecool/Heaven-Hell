#pragma once

#include <memory>

#include "Entity.h"
#include "Vec2.h"

class Quadtree
{
public:
    Quadtree(float x, float y, float width, float height, int capacity);
    void subdivide();
    void insert(Entity entity);
    
    Vec2 getPos() const { return Vec2{m_x, m_y}; }
    Vec2 getSize() const { return Vec2{m_width, m_height}; }

    bool Collision(Entity entity, Quadtree& quadtree);

private:
    int m_capacity = 4;
    
    float m_x;
    float m_y;
    float m_width;
    float m_height;
    
    // Children
    std::unique_ptr<Quadtree> m_northWest;
    std::unique_ptr<Quadtree> m_northEast;
    std::unique_ptr<Quadtree> m_southWest;
    std::unique_ptr<Quadtree> m_southEast;
    
    // Objects
    std::vector<Entity> m_objects;
    
    // Divided
    bool m_divided = false;
    
};

Quadtree::Quadtree(float x, float y, float width, float height, int capacity)
: m_x(x), m_y(y), m_width(width), m_height(height), m_capacity(capacity)
{
    m_objects.reserve(m_capacity);
}

bool Quadtree::Collision(Entity entity, Quadtree& quadtree)
{
    Vec2 entityPos = entity.getComponent<CTransform>().pos;
    Vec2 entityHalfSize = entity.getComponent<CBoundingBox>().halfSize;
    Vec2 treePos = quadtree.getPos();
    Vec2 treeHalfSize = quadtree.getSize();

    bool x_overlap =    (entityPos.x + entityHalfSize.x > treePos.x - treeHalfSize.x) && 
                        (treePos.x + treeHalfSize.x > entityPos.x - entityHalfSize.x);
    if (x_overlap == false) 
    {
        return false;
    }
    else
    {
        bool y_overlap =    (entityPos.y + entityHalfSize.y > treePos.y - treeHalfSize.y) && 
                            (treePos.y + treeHalfSize.y > entityPos.y - entityHalfSize.y);
        return y_overlap;
    }
}
void Quadtree::subdivide()
{
    float subWidth = m_width / 2;
    float subHeight = m_height / 2;
    float x = m_x;
    float y = m_y;
    
    m_northWest = std::make_unique<Quadtree>(x - subWidth, y - subHeight, subWidth, subHeight, m_capacity);
    m_southWest = std::make_unique<Quadtree>(x - subWidth, y + subHeight, subWidth, subHeight, m_capacity);
    m_northEast = std::make_unique<Quadtree>(x + subWidth, y - subHeight, subWidth, subHeight, m_capacity);
    m_southEast = std::make_unique<Quadtree>(x + subWidth, y + subHeight, subWidth, subHeight, m_capacity);
    
    m_divided = true;
}

void Quadtree::insert(Entity entity)
{
    if (Collision(entity, *this) == false)
    {
        return;
    }
    if ((int)m_objects.size() < m_capacity)
    {
        m_objects.push_back(entity);
    }
    else
    {
        if (!m_divided)
        {
            subdivide();
        }
        m_northWest->insert(entity);
        m_northEast->insert(entity);
        m_southWest->insert(entity);
        m_southEast->insert(entity);
    }
}