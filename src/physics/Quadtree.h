#pragma once

#include <memory>

#include "ecs/Entity.h"
#include "physics/Vec2.h"

class Quadtree
{
    int m_capacity = 8;
    
    Vec2 m_position;
    Vec2 m_size;
    
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
    Quadtree(Vec2 pos, Vec2 size);
    void subdivide();
    
    template<typename T>
    void insert(Entity entity)
    {
        if (!Collision<T>(entity, *this)) { return; } // If the entity does not collide with this quadtree, do not insert it
        if (m_divided){
            m_northWest->insert<T>(entity);
            m_northEast->insert<T>(entity);
            m_southWest->insert<T>(entity);
            m_southEast->insert<T>(entity);
            return;
        }
        m_objects.push_back(entity);
        if ((int)m_objects.size() >= m_capacity)
        {  
            if (!m_divided)
            {
                subdivide();
            }
            for (auto entity1 : m_objects)
            {
                m_northWest->insert<T>(entity1);
                m_northEast->insert<T>(entity1);
                m_southWest->insert<T>(entity1);
                m_southEast->insert<T>(entity1);
            }
            m_objects.clear();
        }
    }

    Vec2 getPos() const { return m_position; }
    Vec2 getSize() const { return m_size; }

    template<typename T>
    bool Collision(Entity entity, Quadtree& quadtree)
    {
        Vec2 entityPos = entity.getComponent<CTransform>().pos;
        Vec2 entityHalfSize = entity.getComponent<T>().halfSize;
        Vec2 entitySize = entity.getComponent<T>().size;

        Vec2 treePos = quadtree.getPos();
        Vec2 treeSize = quadtree.getSize();
        Vec2 treeHalfSize = quadtree.getSize()/2;

        Vec2 aPos = entityPos - entityHalfSize;
        Vec2 aSize = entitySize;

        Vec2 bPos = treePos - treeHalfSize;
        Vec2 bSize = treeSize;

        bool x_overlap = (aPos.x + aSize.x > bPos.x) && (bPos.x + bSize.x > aPos.x);
        bool y_overlap = (aPos.y + aSize.y > bPos.y) && (bPos.y + bSize.y > aPos.y);

        return (x_overlap && y_overlap);
    }

    void renderBoundary(SDL_Renderer* renderer, int zoom, Vec2 screenCenter, Vec2 camPos, SDL_Color color);
    int countLeafs(int count );
    std::vector<Entity> getObjects() const;
    std::vector<std::shared_ptr<Quadtree>> createQuadtreeVector();
    void printTree(const std::string& prefix, const std::string& branch);
    bool Divided() const { return m_divided; }
};
