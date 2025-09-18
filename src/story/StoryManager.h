#pragma once
#include "ecs/ECS.hpp"
#include "story/Quest.h"
#include "scenes/Scene_GameOver.h"

#include <unordered_map>
#include <vector>
#include <string>

class Scene_Play;

class Event;

using dialogMap = std::unordered_map<std::string, std::string>;
using NPCDialogs = std::unordered_map<std::string, dialogMap>;

class StoryManager
{
    private:
    Scene_Play* m_scene;
    int m_questID = 0;
    bool m_questsFinished = false;
    std::vector<Quest> m_storyQuests;
    Quest m_currentQuest;
    std::unordered_map<std::string, bool> m_storyFlags;

    NPCDialogs npcDialogs;
    
    public:
    StoryManager() {};
    StoryManager(Scene_Play* scene, std::string storyFilePath);

    void loadStory(const std::string& storyFilePath);
    int getCurrentQuestID();
    void setFlag(const std::string& flagName, bool value);
    void update();
    void onEvent(const Event& e);
    EventType getEventTypeFromString(const std::string& typeStr);
    bool isStoryFinished();
    void Reaction(Quest q);
    std::vector<Quest>& getQuests();
    void loadDialogs(const std::string& path);
    const std::string& getDialog(const std::string& npcId);
};
