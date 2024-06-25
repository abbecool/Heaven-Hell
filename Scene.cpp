// #pragma once

#include "src/include/Scene.h"
// #include "SFML/Graphics/PrimitiveType.hpp"

Scene::Scene() {}

Scene::Scene(Game* game)
: m_game(game) {}

Scene::~Scene() {}

// void Scene::doAction(const Action& action) {
//     sDoAction(action);
// }

void Scene::simulate(const size_t frames) {}

void Scene::registerAction(int inputKey, const std::string& actionName) {
    m_actionMap[inputKey] = actionName;
}

size_t Scene::width() const {
    return m_game->getWidth();
}

size_t Scene::height() const {
    return m_game->getHeight();
}

size_t Scene::currentFrame() const {
    return m_currentFrame;
}

bool Scene::hasEnded() const {
    return m_hasEnded;
}

ActionMap& Scene::getActionMap() {
    return m_actionMap;
}

// void Scene::drawLine(const Vec2& p1, const Vec2& p2) {
//     sf::Vertex line[] = {
//         sf::Vertex(sf::Vector2f(p1.x, p1.y)),
//         sf::Vertex(sf::Vector2f(p2.x, p2.y))
//     };

//     m_game->window().draw(line, 2, sf::Lines);
// }