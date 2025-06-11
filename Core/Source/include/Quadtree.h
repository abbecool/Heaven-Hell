#pragma once

#include <memory>

#include "Entity.h"
#include "Vec2.h"

class Quadtree
{
    int m_capacity = 8;
    
    float m_x;
    float m_y;
    float m_width;
    float m_height;
    
    // Children
    std::shared_ptr<Quadtree> m_northWest;
    std::shared_ptr<Quadtree> m_northEast;
    std::shared_ptr<Quadtree> m_southWest;
    std::shared_ptr<Quadtree> m_southEast;
    
    // Objects
    std::vector<Entity> m_objects;
    
    // Divided
    bool m_divided = false;
public:
    Quadtree(float x, float y, float width, float height);
    void subdivide();
    void insert(Entity entity);
    
    Vec2 getPos() const { return Vec2{m_x, m_y}; }
    Vec2 getSize() const { return Vec2{m_width, m_height}; }

    bool Collision(Entity entity, Quadtree& quadtree);
    void renderBoundary(SDL_Renderer* renderer, int zoom, Vec2 screenCenter, Vec2 camPos);
    int countLeafs(int count );
    std::vector<Entity> getObjects() const;
    std::vector<std::shared_ptr<Quadtree>> createQuadtreeVector();
    void printTree(const std::string& prefix, const std::string& branch);
    bool Divided() const { return m_divided; }
};
