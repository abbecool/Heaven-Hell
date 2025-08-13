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
        StoryStep storyStep;
        storyStep.id = step["id"];
        storyStep.description = step["description"];
        storyStep.triggerType = step["trigger"]["type"];
        storyStep.triggerType = step["trigger"]["type"];
        storyStep.triggerType = step["trigger"]["type"];

        if (step.contains("on_complete")) {
            storyStep.onCompleteType = step["on_complete"]["type"];
            if (storyStep.onCompleteType == "flag") {
                storyStep.onCompleteName = step["on_complete"]["name"];
                storyStep.onCompleteValue = step["on_complete"]["value"];
            }
            else if (storyStep.onCompleteType == "spawn") {
                storyStep.onCompleteEntity = step["on_complete"]["entity"];
                storyStep.onCompleteLocation = Vec2(step["on_complete"]["location"]["x"], step["on_complete"]["location"]["y"]);
            }
        }

        // storyStep.setTrigger(step["trigger"]);
        // storyStep.setOnComplete(step["on_complete"]);

        m_storySteps.push_back(storyStep);
    }
}

void StoryManager::getCurrentQuest()
{
    std::cout << "Current quest: " << m_progression << std::endl;
}


    // ,
    // {
    //   "id": 1,
    //   "description": "Defeat the forest rooter.",
    //   "trigger": {
    //     "type": "event",
    //     "name": "enemy_killed",
    //     "target": "forest_rooter"
    //   },
    //   "on_complete": {
    //     "type": "flag",
    //     "name": "rooter_defeated",
    //     "value": true
    //   }
    // }