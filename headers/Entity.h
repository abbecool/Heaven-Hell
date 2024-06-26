#include <iostream>
#include <memory>
#include <SDL2/SDL.h>
#include "CComponents.h"

#ifndef ENTITY_H
#define ENTITY_H 

class Entity
{
    const size_t        m_id    = 0;
    const std::string   m_tag   = "Default";
    bool                m_alive = true;
public:
    std::shared_ptr<CTransform> cTransform;
    std::shared_ptr<CName> cName;
    std::shared_ptr<CShape> cShape;
    std::shared_ptr<CInputs> cInputs;
    std::shared_ptr<CKey> cKey;
    std::shared_ptr<CTexture> cTexture;
    Entity(const std::string& tag, const size_t id)
        : m_tag(tag)
        , m_id(id)
        {

        }
    size_t getId();
    const std::string tag();
    bool isAlive();
    void kill();
    void movePosition(Vec2);
    void setColor(const int r, const int g, const int b, const int a);
};

size_t Entity::getId()
{
    return m_id;
}

const std::string Entity::tag()
{
    return m_tag;
}

bool Entity::isAlive()
{
    return m_alive;
}

void Entity::kill()
{
    m_alive = false;
}

void Entity::movePosition(Vec2 move)
{
    cTransform->pos.x += move.x;
    cTransform->pos.y += move.y;
}

void Entity::setColor(const int r, const int g, const int b, const int a)
{
    cShape->r_val = r;
    cShape->g_val = g;
    cShape->b_val = b;
    cShape->a_val = a;
}

#endif // ENTITY_H