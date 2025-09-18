#include "story/StoryManager.h"
#include "scenes/Scene_Play.h"
#include "story/EventBus.h"
#include <fstream>

#include "external/json.hpp"
using json = nlohmann::json;

StoryManager::StoryManager(Scene_Play* scene, std::string storyFilePath)
{
    m_scene = scene;
    loadStory(storyFilePath);
    loadDialogs("config_files/dialogs.json");
    m_currentQuest = m_storyQuests[m_questID];
}

void StoryManager::loadStory(const std::string& storyFilePath)
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
        m_storyQuests.push_back(quest);
    }
}

int StoryManager::getCurrentQuestID()
{
    return m_questID;
}

void StoryManager::setFlag(const std::string& flagName, bool value)
{
    m_storyFlags[flagName] = true;
}

void StoryManager::onEvent(const Event& e) {
    Quest quest = m_currentQuest;
    QuestStep& step = quest.steps[quest.currentStep];
    if (!step.matches(e)){ return; }
    step.completed = true;

    Reaction(quest);
    
    int nextID = quest.id + 1;
    if (nextID == m_storyQuests.size())
    {
        m_questsFinished = true;
        return;
    } 
    m_currentQuest = m_storyQuests[nextID];
    std::cout << "New quest: " << m_currentQuest.name << std::endl;
}

void StoryManager::Reaction(Quest quest){
    Event event = quest.onComplete;
    switch (event.type)
    {
    case EventType::EntitySpawned:
        m_scene->Spawn(event.itemName, event.eventPosition);
        break;
    case EventType::FlagChanged:
        setFlag(quest.name, true);
        break;
    default:
        break;
    }
}

bool StoryManager::isStoryFinished(){
    return m_questsFinished;
}

EventType StoryManager::getEventTypeFromString(const std::string& typeStr) {
    if (typeStr == "ItemPickedUp") return EventType::ItemPickedUp;
    if (typeStr == "EntityKilled") return EventType::EntityKilled;
    if (typeStr == "EntitySpawned") return EventType::EntitySpawned;
    if (typeStr == "DialogueFinished") return EventType::DialogueFinished;
    if (typeStr == "FlagChanged") return EventType::FlagChanged;
    if (typeStr == "EnteredArea") return EventType::EnteredArea;
    throw std::runtime_error("Unknown event type: " + typeStr);
}

std::vector<Quest>& StoryManager::getQuests(){
    return m_storyQuests;
}

void StoryManager::loadDialogs(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open dialogs file: " << path << "\n";
        return;
    }

    json j;
    file >> j;

    for (auto& [npcId, dialogArray] : j["dialogs"].items()) {
        std::unordered_map<int, std::string> questDialogMap;

        for (auto& obj : dialogArray) {
            // Each element in array is an object like { "1": "first line wiz" }
            for (auto& [questIdStr, line] : obj.items()) {
                int questId = std::stoi(questIdStr);
                questDialogMap[questId] = line.get<std::string>();
            }
        }

        npcDialogs[npcId] = questDialogMap;
    }
}

const std::string& StoryManager::getDialog(const std::string& npcId) {
    auto it = npcDialogs[npcId].find(m_currentQuest.id);
    if (it == npcDialogs[npcId].end()) {
        static const std::string empty = "I have nothing to say right now...";
        return empty;
    }
    return it->second;
}
