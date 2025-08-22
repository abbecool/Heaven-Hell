#pragma once
#include "ecs/ECS.hpp"
#include "story/StoryQuest.h"

#include <unordered_map>
#include <vector>
#include <string>

class Scene_Play;

class GameEvent;

class StoryManager
{
    private:
    ECS* m_ECS;
    Scene_Play* m_scene;
    int m_questID = 0;
    std::vector<StoryQuest> m_storyQuests;
    StoryQuest m_currentQuest;
    std::unordered_map<std::string, bool> m_storyFlags;
    
    public:
    StoryManager() {};
    StoryManager(ECS* ecs, Scene_Play* scene, std::string storyFilePath);

    void loadStory(const std::string& storyFilePath);
    int getCurrentQuestID();
    void setFlag(const std::string& flagName, bool value);
    void update();
    void onEvent(const GameEvent& e);
    void completeQuest(StoryQuest quest);
};
