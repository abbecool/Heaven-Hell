#pragma once

// #include <iostream>
#include <memory>
// #include <SDL2/SDL.h>
#include "Components.h"
#include <tuple>
#include <string>

class Entity
{
    const std::string   m_tag   = "Default";
    const size_t        m_id    = 0;
    const size_t        m_layer    = 0;
    bool                m_alive = true;
public:
    std::shared_ptr<CTransform> cTransform;
    std::shared_ptr<CName> cName;
    std::shared_ptr<CShape> cShape;
    std::shared_ptr<CInputs> cInputs;
    std::shared_ptr<CKey> cKey;
    std::shared_ptr<CTexture> cTexture;
    std::shared_ptr<CAnimation> cAnimation;
    Entity(const std::string& tag, const size_t id, const size_t layer);
    size_t getId();
    const std::string tag();
    const size_t layer();
    bool isAlive();
    void kill();
    void movePosition(Vec2);
    void setColor(const int r, const int g, const int b, const int a);
};
