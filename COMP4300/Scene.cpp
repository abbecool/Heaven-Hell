#include "Scene.h"

Scene::Scene()
{
}

Scene::Scene(GameEngine* gameEngine)
{
}

Scene::~Scene()
{
}

void Scene::doAction(const Action& action)
{
}

void Scene::simulate(const size_t frames)
{
}


size_t Scene::width() const
{
	return 0;
}

size_t Scene::height() const
{
	return 0;
}

size_t Scene::currentFrame() const
{
	return 0;
}

bool Scene::hasEnded() const
{
	return false;
}


void Scene::drawLine(const Vec2& p1, const Vec2& p2)
{

}

void Scene::setPaused(bool paused)
{

}


/* handle actions */


void Scene::registerAction(int inputKey, const std::string& actionName)
{
	m_actionMap[inputKey] = actionName;
}

ActionMap& Scene::getActionMap()
{
	return m_actionMap;
}