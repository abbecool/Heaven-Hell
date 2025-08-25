#pragma once
#include "ecs/ECS.hpp"
#include "story/Quest.h"
#include "scenes/Scene_GameOver.h"

#include <unordered_map>
#include <vector>
#include <string>

class Scene_Play;

class Event;

class StoryManager
{
    private:
    Scene_Play* m_scene;
    int m_questID = 0;
    bool m_questsFinished = false;
    std::vector<Quest> m_storyQuests;
    Quest m_currentQuest;
    std::unordered_map<std::string, bool> m_storyFlags;
    
    public:
    StoryManager() {};
    StoryManager(Scene_Play* scene, std::string storyFilePath);

    void loadStory(const std::string& storyFilePath);
    int getCurrentQuestID();
    void setFlag(const std::string& flagName, bool value);
    void update();
    void onEvent(const Event& e);
    EventType getEventTypeFromString(const std::string& typeStr);
    bool IsStoryFinished();
    void Reaction(Quest q);
};
