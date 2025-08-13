#pragma once
#include "ecs/ECS.hpp"
#include "scenes/Scene.h"
#include "story/StoryStep.h"

#include <unordered_map>
#include <vector>
#include <string>

using Progression = uint8_t;

class StoryManager
{
    private:
    ECS* m_ECS;
    Scene* m_scene;
    Progression m_progression = 0;
    std::vector<StoryStep> m_storySteps;
    
    public:
    StoryManager() {};
    StoryManager(ECS* ecs, Scene* scene, std::string storyFilePath);
    void loadStory(const std::string& storyFilePath);
    void getCurrentQuest();
};
