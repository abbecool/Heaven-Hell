#include "Quadtree.hpp"

Quadtree::Quadtree(Vec2 pos, Vec2 size)
: m_position(pos), m_size(size)
{
    m_objects.reserve(m_capacity);
}

void Quadtree::subdivide()
{
    Vec2 subSize = m_size / 2;
    float x = m_position.x;
    float y = m_position.y;
    
    m_northWest = std::make_unique<Quadtree>(Vec2{x - subSize.x/2, y - subSize.y/2}, subSize);
    m_southWest = std::make_unique<Quadtree>(Vec2{x - subSize.x/2, y + subSize.y/2}, subSize);
    m_northEast = std::make_unique<Quadtree>(Vec2{x + subSize.x/2, y - subSize.y/2}, subSize);
    m_southEast = std::make_unique<Quadtree>(Vec2{x + subSize.x/2, y + subSize.y/2}, subSize);
    
    m_divided = true;
}

void Quadtree::printTree(const std::string& prefix, const std::string& branch)
{
    std::cout << prefix << branch;

    if (prefix.empty()) {
        std::cout << "root";
    }
    std::cout << std::endl;

    // Print entities in this node
    for (auto& object : m_objects) {
        std::cout << prefix << "  --object" << object.index << std::endl;
    }

    // Print children recursively
    if (m_divided) {
        m_northWest->printTree(prefix + "  ", "-nw ");
        m_northEast->printTree(prefix + "  ", "-ne ");
        m_southWest->printTree(prefix + "  ", "-sw ");
        m_southEast->printTree(prefix + "  ", "-se ");
    }
}

void Quadtree::renderBoundary(
    RenderBackend& renderer,
    Color color
) {
    if (m_divided)
    {
        m_northWest->renderBoundary(renderer, color);
        m_northEast->renderBoundary(renderer, color);
        m_southWest->renderBoundary(renderer, color);
        m_southEast->renderBoundary(renderer, color);
    }
    else
    {
        float x = m_position.x;
        float y = m_position.y;
        float width = m_size.x;
        float height = m_size.y;
        RectF collisionRect{
            x - width / 2,
            y - height / 2,
            width,
            height
        };

        renderer.drawWorldRect(collisionRect, color);
        // render the number of objects in the quadtree
    }
}

int Quadtree::countLeafs(int count)
{
    if (m_divided)
    {
        count += m_northWest->countLeafs(0);
        count += m_northEast->countLeafs(0);
        count += m_southWest->countLeafs(0);
        count += m_southEast->countLeafs(0);
    }
    else
    {
        count++;
    }
    return count;
}

std::vector<size_t> Quadtree::getObjects() const
{ 
    std::vector<size_t> objects;
    objects.reserve(m_objects.size());
    for (const auto& object : m_objects) {
        objects.push_back(object.index);
    }
    return objects;
}

std::vector<std::shared_ptr<Quadtree>> Quadtree::createQuadtreeVector()
{
    std::vector<std::shared_ptr<Quadtree>> quadtreeVector;
    if (m_divided)
    {        
        auto nwVector = m_northWest->createQuadtreeVector();
        auto neVector = m_northEast->createQuadtreeVector();
        auto swVector = m_southWest->createQuadtreeVector();
        auto seVector = m_southEast->createQuadtreeVector();
        
        quadtreeVector.insert(quadtreeVector.end(), nwVector.begin(), nwVector.end());
        quadtreeVector.insert(quadtreeVector.end(), neVector.begin(), neVector.end());
        quadtreeVector.insert(quadtreeVector.end(), swVector.begin(), swVector.end());
        quadtreeVector.insert(quadtreeVector.end(), seVector.begin(), seVector.end());
    }
    else
    {
        quadtreeVector.push_back(std::shared_ptr<Quadtree>(this, [](Quadtree*){}));
    }
    
    return quadtreeVector;
}
