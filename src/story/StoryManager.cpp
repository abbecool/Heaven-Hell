#include "story/StoryManager.h"
#include <fstream>

#include "external/json.hpp"
using json = nlohmann::json;

StoryManager::StoryManager(ECS* ecs, Scene* scene, std::string storyFilePath)
{
    m_ECS = ecs;
    m_scene = scene;
    loadStory(storyFilePath);
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
                storyQuest.onCompleteLocation = Vec2(step["on_complete"]["location"]["x"], step["on_complete"]["location"]["y"]);
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
