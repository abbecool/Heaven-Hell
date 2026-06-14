#include "Quadtree.h"

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
    for (auto& entity : m_objects) {
        std::cout << prefix << "  --entity" << entity.getID() << std::endl;
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
    int zoom, 
    Vec2 screenCenter, 
    Vec2 camPos, 
    Color color
) {
    if (m_divided)
    {
        m_northWest->renderBoundary(renderer, zoom, screenCenter, camPos, color);
        m_northEast->renderBoundary(renderer, zoom, screenCenter, camPos, color);
        m_southWest->renderBoundary(renderer, zoom, screenCenter, camPos, color);
        m_southEast->renderBoundary(renderer, zoom, screenCenter, camPos, color);
    }
    else
    {
        float x = m_position.x;
        float y = m_position.y;
        float width = m_size.x;
        float height = m_size.y;
        // Adjust the collision box position based on the camera position
        RectF collisionRect{
            (x - width/2 - camPos.x) * zoom + screenCenter.x,
            (y - height/2 - camPos.y) * zoom + screenCenter.y,
            width * zoom,
            height * zoom
        };

        renderer.drawRect(collisionRect, color);
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

std::vector<Entity> Quadtree::getObjects() const
{ 
    return m_objects;
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
