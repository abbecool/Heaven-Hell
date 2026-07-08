#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "physics/Vec2.hpp"
#include "render/RenderBackend.hpp"

class Quadtree
{
    struct ObjectBounds
    {
        size_t index = 0;
        Vec2 center = {0, 0};
        Vec2 size = {0, 0};
    };

    size_t m_capacity = 8;
    
    Vec2 m_position;
    Vec2 m_size;
    
    // Children
    std::shared_ptr<Quadtree> m_northWest;
    std::shared_ptr<Quadtree> m_northEast;
    std::shared_ptr<Quadtree> m_southWest;
    std::shared_ptr<Quadtree> m_southEast;
    
    // Objects
    std::vector<ObjectBounds> m_objects;
    
    // Divided
    bool m_divided = false;
public:
    Quadtree(Vec2 pos, Vec2 size);
    void subdivide();
    void clear();
    std::unique_ptr<Quadtree> clone() const;
    
    void insert(size_t objectIndex, Vec2 objectCenter, Vec2 objectSize)
    {
        if (m_size.x < 32){
            return;
        }
        if (!intersects(objectCenter, objectSize, m_position, m_size)) { return; }
        if (m_divided) {
            m_northWest->insert(objectIndex, objectCenter, objectSize);
            m_northEast->insert(objectIndex, objectCenter, objectSize);
            m_southWest->insert(objectIndex, objectCenter, objectSize);
            m_southEast->insert(objectIndex, objectCenter, objectSize);
            return;
        }
        m_objects.push_back(ObjectBounds{objectIndex, objectCenter, objectSize});
        if (m_objects.size() > m_capacity)
        {
            if (!m_divided)
            {
                subdivide();
            }
            auto objects = m_objects;
            m_objects.clear();
            for (auto object : objects)
            {
                m_northWest->insert(object.index, object.center, object.size);
                m_northEast->insert(object.index, object.center, object.size);
                m_southWest->insert(object.index, object.center, object.size);
                m_southEast->insert(object.index, object.center, object.size);
            }
        }
    }

    Vec2 getPos() const { return m_position; }
    Vec2 getSize() const { return m_size; }

    static bool intersects(Vec2 objectCenter, Vec2 objectSize, Vec2 treeCenter, Vec2 treeSize)
    {
        Vec2 objectHalfSize = objectSize / 2;
        Vec2 treeHalfSize = treeSize / 2;

        Vec2 aPos = objectCenter - objectHalfSize;
        Vec2 aSize = objectSize;

        Vec2 bPos = treeCenter - treeHalfSize;
        Vec2 bSize = treeSize;

        bool x_overlap = (aPos.x + aSize.x > bPos.x) && (bPos.x + bSize.x > aPos.x);
        bool y_overlap = (aPos.y + aSize.y > bPos.y) && (bPos.y + bSize.y > aPos.y);

        return (x_overlap && y_overlap);
    }

    void renderBoundary(RenderBackend& renderer, Color color);
    int countLeafs(int count );
    std::vector<size_t> getObjects() const;
    std::vector<std::shared_ptr<Quadtree>> createQuadtreeVector();
    void printTree(const std::string& prefix, const std::string& branch);
    bool Divided() const { return m_divided; }
};
