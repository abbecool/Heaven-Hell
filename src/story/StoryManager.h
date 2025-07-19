#pragma once
#include "ecs/ECS.hpp"
#include "scenes/Scene.h"

using Progression = uint8_t;

class StoryManager
{
    private:
    ECS* m_ECS;
    Scene* m_scene;
    Progression m_progression = 0;
    
    public:
    StoryManager() {};
    StoryManager(ECS* ecs, Scene* scene);
    std::string getDialog();
    Progression getProgression();
    void updateProgression();
};