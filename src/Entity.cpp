#include "Entity.h"
#include <string>

Entity::Entity(const std::string& tag, const size_t id, const size_t layer)
        : m_tag(tag), m_id(id), m_layer(layer){}

size_t Entity::Entity::getId(){
    return m_id;
}

const std::string Entity::Entity::tag(){
    return m_tag;
}

const size_t Entity::Entity::layer(){
    return m_layer;
}

bool Entity::Entity::isAlive(){
    return m_alive;
}

void Entity::Entity::kill(){
    m_alive = false;
}

void Entity::Entity::movePosition(Vec2 move){
    cTransform->pos.x += move.x;
    cTransform->pos.y += move.y;
}

void Entity::Entity::setColor(const int r, const int g, const int b, const int a){
    cShape->r_val = r;
    cShape->g_val = g;
    cShape->b_val = b;
    cShape->a_val = a;
}
