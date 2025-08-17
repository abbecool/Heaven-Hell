#include "story/StoryManager.h"
#include "scenes/Scene_Play.h"
#include <fstream>

#include "external/json.hpp"
using json = nlohmann::json;

StoryManager::StoryManager(ECS* ecs, Scene_Play* scene, std::string storyFilePath)
{
    m_ECS = ecs;
    m_scene = scene;
    loadStory(storyFilePath);
    m_currentQuest = m_storyQuests[m_questID];
}

void StoryManager::loadStory(const std::string& storyFilePath)
{
    std::ifstream file_assets(storyFilePath);
    if (!file_assets) {
        std::cerr << "Could not load assets file!\n";
        exit(-1);
    }
    json j;
    file_assets >> j;
    file_assets.close();

    for (const auto& step : j["story_steps"]) {
        StoryQuest storyQuest;
        storyQuest.id = step["id"];
        storyQuest.description = step["description"];
        storyQuest.triggerType = step["trigger"]["type"];
        storyQuest.triggerName = step["trigger"]["name"];

        if (step.contains("on_complete")) {
            storyQuest.onCompleteType = step["on_complete"]["type"];
            if (storyQuest.onCompleteType == "flag") {
                storyQuest.onCompleteName = step["on_complete"]["name"];
            }
            else if (storyQuest.onCompleteType == "spawn") {
                storyQuest.onCompleteEntity = step["on_complete"]["entity"];
                storyQuest.onCompleteposition = Vec2(step["on_complete"]["position"]);
            }
        }
        m_storyQuests.push_back(storyQuest);
    }
}

int StoryManager::getCurrentQuestID()
{
    return m_questID;
}

void StoryManager::setFlag(const std::string& flagName, bool value)
{
    if (m_currentQuest.triggerType == "flag" && m_currentQuest.triggerName == flagName) {
        m_currentQuest.triggerValue = value;
    }
}

void StoryManager::update()
{
    if (m_questID >= m_storyQuests.size()) {
        std::cout << "All quests completed!" << std::endl;
    }
    StoryQuest quest = m_currentQuest;
    if (quest.triggerType == "flag" && quest.triggerValue) {
        m_questID++;
        if (quest.onCompleteType == "flag") {
            setFlag(quest.onCompleteName, true);
        }
        else if (quest.onCompleteType == "spawn") {
            EntityID id = m_scene->Spawn(quest.onCompleteEntity, quest.onCompleteposition);
        }
        m_currentQuest = m_storyQuests[m_questID];
    }
}
