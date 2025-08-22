#include "story/StoryManager.h"
#include "scenes/Scene_Play.h"
#include "story/EventBus.h"
#include <fstream>

#include "external/json.hpp"
using json = nlohmann::json;

StoryManager::StoryManager(ECS* ecs, Scene_Play* scene, std::string storyFilePath)
{
    m_ECS = ecs;
    m_scene = scene;
    loadStory(storyFilePath);
    loadStory1("config_files/story1.json");
    m_currentQuest = m_storyQuests[m_questID];
    m_currentQuest1 = m_storyQuests1[m_questID];
}

void StoryManager::loadStory1(const std::string& storyFilePath)
{
    std::ifstream file_assets(storyFilePath);
    if (!file_assets) {
        std::cerr << "Could not load new story file!\n";
        exit(-1);
    }
    json j;
    file_assets >> j;
    file_assets.close();

    for (const auto& step : j["story_steps"]) {
        Quest quest;
        QuestStep stepQuest;
        quest.id = step["id"].get<int>();
        quest.name = step["description"];
        
        stepQuest.requiredType = getEventTypeFromString(step["trigger"]["event_type"].get<std::string>());

        stepQuest.requiredSubject = step["trigger"]["subject"].get<std::string>();
        
        quest.steps.push_back(stepQuest);

        if (step.contains("reaction")) {
            quest.onComplete = Event{
                getEventTypeFromString(step["reaction"]["event_type"].get<std::string>()),
                step["reaction"]["subject"].get<std::string>(),
                Vec2(step["reaction"]["position"])
            };            
        }
        m_storyQuests1.push_back(quest);
    }
}

void StoryManager::loadStory(const std::string& storyFilePath)
{
    std::ifstream file_assets(storyFilePath);
    if (!file_assets) {
        std::cerr << "Could not load story file!\n";
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

        if (step.contains("reaction")) {
            storyQuest.onCompleteType = step["reaction"]["type"];
            if (storyQuest.onCompleteType == "flag") {
                storyQuest.onCompleteName = step["reaction"]["name"];
            }
            else if (storyQuest.onCompleteType == "spawn") {
                storyQuest.onCompleteEntity = step["reaction"]["entity"];
                storyQuest.onCompleteposition = Vec2(step["reaction"]["position"]);
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
    m_storyFlags[flagName] = true;
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

void StoryManager::onEvent(const Event& e) {
    Quest quest = m_currentQuest1;
    QuestStep step = quest.steps[quest.currentStep];
    if (!step.matches(e)){ return; }

    if (quest.onComplete.type == EventType::EntitySpawned){
        EntityID id = m_scene->Spawn(quest.onComplete.itemName, quest.onComplete.eventPosition);
    }
}

void StoryManager::completeQuest(StoryQuest quest){
    if (m_questID >= m_storyQuests.size()) {
        std::cout << "All quests completed!" << std::endl;
    }

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

EventType StoryManager::getEventTypeFromString(const std::string& typeStr) {
    if (typeStr == "ItemPickedUp") return EventType::ItemPickedUp;
    if (typeStr == "EntityKilled") return EventType::EntityKilled;
    if (typeStr == "EntitySpawned") return EventType::EntitySpawned;
    if (typeStr == "DialogueFinished") return EventType::DialogueFinished;
    if (typeStr == "FlagChanged") return EventType::FlagChanged;
    throw std::runtime_error("Unknown event type: " + typeStr);
}