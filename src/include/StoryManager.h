#pragma once
#include "ECS.hpp"
#include "Scene.h"

using Progression = uint8_t;

class StoryManager
{
    private:
    ECS* m_ECS;
    Scene* m_scene;
    Progression m_progression = 0;
    
    public:
    std::string getDialog();
    int getProgression();
    void updateProgression();
}