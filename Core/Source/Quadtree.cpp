#include "Quadtree.h"

Quadtree::Quadtree(float x, float y, float width, float height)
: m_x(x), m_y(y), m_width(width), m_height(height)
{
    m_objects.reserve(m_capacity);
}

// template<typename T>
// bool Quadtree::Collision(Entity entity, Quadtree& quadtree)
// {
//     Vec2 entityPos = entity.getComponent<CTransform>().pos;
//     Vec2 entityHalfSize = entity.getComponent<T>().halfSize;
//     Vec2 entitySize = entity.getComponent<T>().size;

//     Vec2 treePos = quadtree.getPos();
//     Vec2 treeSize = quadtree.getSize();
//     Vec2 treeHalfSize = quadtree.getSize()/2;

//     Vec2 aPos = entityPos - entityHalfSize;
//     Vec2 aSize = entitySize;

//     Vec2 bPos = treePos - treeHalfSize;
//     Vec2 bSize = treeSize;

//     bool x_overlap = (aPos.x + aSize.x > bPos.x) && (bPos.x + bSize.x > aPos.x);
//     bool y_overlap = (aPos.y + aSize.y > bPos.y) && (bPos.y + bSize.y > aPos.y);

//     return (x_overlap && y_overlap);
    
//     // entityPos.print("entityPos");
//     // entityHalfSize.print("entityHalfSize");
//     // treePos.print("treePos");
//     // treeHalfSize.print("treeHalfSize");
    
//     // bool x_overlap =    (entityPos.x + entityHalfSize.x > treePos.x - treeHalfSize.x) && 
//     // (treePos.x + treeHalfSize.x > entityPos.x - entityHalfSize.x);

//     // bool y_overlap =    (entityPos.y + entityHalfSize.y > treePos.y - treeHalfSize.y) && 
//     // (treePos.y + treeHalfSize.y > entityPos.y - entityHalfSize.y);
//     // std::cout << entity.getID() << ". Coll: " << (x_overlap & y_overlap) << ". treePos: " << treePos.x << ", " << treePos.y << ". entityPos: " << entityPos.x << ", " << entityPos.y << "\n";
//     // std::cout << entity.getID() << ". Coll: " << (x_overlap & y_overlap) << ". treeHalfSize: " << treeHalfSize.x << ", " << treeHalfSize.y << ". entityHalfSize: " << entityHalfSize.x << ", " << entityHalfSize.y << "\n";
//     // return x_overlap & y_overlap;

// }

void Quadtree::subdivide()
{
    float subWidth = m_width / 2;
    float subHeight = m_height / 2;
    float x = m_x;
    float y = m_y;
    
    m_northWest = std::make_unique<Quadtree>(x - subWidth/2, y - subHeight/2, subWidth, subHeight);
    m_southWest = std::make_unique<Quadtree>(x - subWidth/2, y + subHeight/2, subWidth, subHeight);
    m_northEast = std::make_unique<Quadtree>(x + subWidth/2, y - subHeight/2, subWidth, subHeight);
    m_southEast = std::make_unique<Quadtree>(x + subWidth/2, y + subHeight/2, subWidth, subHeight);
    
    m_divided = true;
    // std::cout << "subdivide" << std::endl;
}

// template<typename T>
// void Quadtree::insert(Entity entity)
// {
//     if (!Collision<T>(entity, *this)) { return; } // If the entity does not collide with this quadtree, do not insert it
//     // std::cout << "insert" << std::endl;

//     if (m_divided){
//         // If the node is divided, try insert into the child nodes
//         m_northWest->insert<T>(entity);
//         m_northEast->insert<T>(entity);
//         m_southWest->insert<T>(entity);
//         m_southEast->insert<T>(entity);
//         return; // No need to insert into this node, as it is already divided
//     }
//     m_objects.push_back(entity);
//     // std::cout << (int)m_objects.size() << " | " << m_capacity << std::endl;
//     if ((int)m_objects.size() >= m_capacity)
//     {  
//         if (!m_divided)
//         {
//             subdivide();
//         }
//         for (auto entity1 : m_objects)
//         {
//             m_northWest->insert<T>(entity1);
//             m_northEast->insert<T>(entity1);
//             m_southWest->insert<T>(entity1);
//             m_southEast->insert<T>(entity1);
//         }
//         m_objects.clear(); // Clear the objects in this node after subdividing
//     }
// }

void Quadtree::printTree(const std::string& prefix, const std::string& branch)
{
    std::cout << prefix << branch;

    if (prefix.empty()) {
        std::cout << "root";
    // } else if (branch == "-nw ") {
    //     std::cout << "nw";
    // } else if (branch == "-ne ") {
    //     std::cout << "ne";
    // } else if (branch == "-sw ") {
    //     std::cout << "sw";
    // } else if (branch == "-se ") {
    //     std::cout << "se";
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

void Quadtree::renderBoundary(SDL_Renderer* renderer, int zoom, Vec2 screenCenter, Vec2 camPos, SDL_Color color)
{
    if (m_divided)
    {
        m_northWest->renderBoundary(renderer, zoom, screenCenter, camPos, color);
        m_northEast->renderBoundary(renderer, zoom, screenCenter, camPos, color);
        m_southWest->renderBoundary(renderer, zoom, screenCenter, camPos, color);
        m_southEast->renderBoundary(renderer, zoom, screenCenter, camPos, color);
    }
    else
    {
        // Adjust the collision box position based on the camera position
        SDL_Rect collisionRect;
        collisionRect.x = int ( m_x - m_width/2 - camPos.x) * zoom + screenCenter.x;
        collisionRect.y = int ( m_y - m_height/2 - camPos.y) * zoom + screenCenter.y;
        collisionRect.w = int ( m_width ) * zoom;
        collisionRect.h = int ( m_height ) * zoom;

        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderDrawRect(renderer, &collisionRect);
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
    // std::cout << "countLeafs: " << count << " | " << m_divided << std::endl;
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
