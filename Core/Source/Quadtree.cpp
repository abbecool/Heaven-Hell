#include "Quadtree.h"

Quadtree::Quadtree(float x, float y, float width, float height)
: m_x(x), m_y(y), m_width(width), m_height(height)
{
    m_objects.reserve(m_capacity);
}

bool Quadtree::Collision(Entity entity, Quadtree& quadtree)
{
    Vec2 entityPos = entity.getComponent<CTransform>().pos;
    Vec2 entityHalfSize = entity.getComponent<CCollisionBox>().halfSize;
    Vec2 treePos = quadtree.getPos();
    Vec2 treeHalfSize = quadtree.getSize();
    
    // entityPos.print("entityPos");
    // entityHalfSize.print("entityHalfSize");
    // treePos.print("treePos");
    // treeHalfSize.print("treeHalfSize");

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
    
    m_northWest = std::make_unique<Quadtree>(x - subWidth, y - subHeight, subWidth, subHeight);
    m_southWest = std::make_unique<Quadtree>(x - subWidth, y + subHeight, subWidth, subHeight);
    m_northEast = std::make_unique<Quadtree>(x + subWidth, y - subHeight, subWidth, subHeight);
    m_southEast = std::make_unique<Quadtree>(x + subWidth, y + subHeight, subWidth, subHeight);
    
    m_divided = true;
    // std::cout << "subdivide" << std::endl;
}

void Quadtree::insert(Entity entity)
{
    if (!Collision(entity, *this))
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
    // std::cout << "test" << std::endl;
}

void Quadtree::renderBoundary(SDL_Renderer* renderer, int zoom, Vec2 screenCenter, Vec2 camPos)
{
    if (m_divided)
    {
        m_northWest->renderBoundary(renderer, zoom, screenCenter, camPos);
        m_northEast->renderBoundary(renderer, zoom, screenCenter, camPos);
        m_southWest->renderBoundary(renderer, zoom, screenCenter, camPos);
        m_southEast->renderBoundary(renderer, zoom, screenCenter, camPos);
    }
    else
    {
        // Adjust the collision box position based on the camera position
        SDL_Rect collisionRect;
        collisionRect.x = int ( m_x - m_width - camPos.x) * zoom + screenCenter.x;
        collisionRect.y = int ( m_y - m_height - camPos.y) * zoom + screenCenter.y;
        collisionRect.w = int ( m_width*2 ) * zoom;
        collisionRect.h = int ( m_height*2 ) * zoom;
    
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
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