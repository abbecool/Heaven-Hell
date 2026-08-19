#include "story/StoryManager.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {
constexpr const char* MainTrack = "main";
}

StoryManager::StoryManager(Scene_Play* scene, std::string storyFilePath, std::string questFilePath)
{
    (void)scene;
    loadStory(storyFilePath);
    loadQuests(questFilePath);
    loadDialogs("config_files/dialogs.json");
}

void StoryManager::loadStory(const std::string& storyFilePath)
{
    loadQuestFile(storyFilePath);
}

void StoryManager::loadQuests(const std::string& questFilePath)
{
    loadQuestFile(questFilePath);
}

void StoryManager::loadQuestFile(const std::string& questFilePath)
{
    std::ifstream file(questFilePath);
    if (!file) {
        throw std::runtime_error("Could not load quest file: " + questFilePath);
    }

    json data;
    file >> data;

    const json* questsJson = &data;
    if (data.contains("quests")) {
        questsJson = &data.at("quests");
    }

    if (!questsJson->is_array()) {
        throw std::runtime_error("Quest file must contain a quest array: " + questFilePath);
    }

    for (const json& questJson : *questsJson) {
        addQuest(parseQuest(questJson));
    }
}

Quest StoryManager::parseQuest(const json& questJson) const
{
    Quest quest;
    quest.id = questJson.at("id").get<std::string>();
    quest.title = questJson.value("title", quest.id);
    quest.track = questJson.value("track", MainTrack);
    quest.state = getQuestStateFromString(questJson.value("state", "locked"));

    for (const json& stepJson : questJson.at("steps")) {
        quest.steps.push_back(parseStep(stepJson));
    }

    if (questJson.contains("reaction")) {
        quest.reactions.push_back(parseReaction(questJson.at("reaction")));
    }
    if (questJson.contains("reactions")) {
        for (const json& reactionJson : questJson.at("reactions")) {
            quest.reactions.push_back(parseReaction(reactionJson));
        }
    }

    return quest;
}

QuestStep StoryManager::parseStep(const json& stepJson) const
{
    QuestStep step;
    step.id = stepJson.at("id").get<std::string>();
    step.text = stepJson.value("text", step.id);
    step.trigger = parseTrigger(stepJson.at("trigger"));

    if (stepJson.contains("reaction")) {
        step.reactions.push_back(parseReaction(stepJson.at("reaction")));
    }
    if (stepJson.contains("reactions")) {
        for (const json& reactionJson : stepJson.at("reactions")) {
            step.reactions.push_back(parseReaction(reactionJson));
        }
    }

    return step;
}

QuestTrigger StoryManager::parseTrigger(const json& triggerJson) const
{
    QuestTrigger trigger;
    trigger.eventType = getEventTypeFromString(triggerJson.at("event_type").get<std::string>());

    if (triggerJson.contains("subjects")) {
        for (const json& subjectJson : triggerJson.at("subjects")) {
            trigger.subjects.push_back(subjectJson.get<std::string>());
        }
    }
    else if (triggerJson.contains("subject")) {
        trigger.subjects.push_back(triggerJson.at("subject").get<std::string>());
    }

    return trigger;
}

QuestReaction StoryManager::parseReaction(const json& reactionJson) const
{
    QuestReaction reaction;
    reaction.type = getReactionTypeFromString(reactionJson.at("type").get<std::string>());

    switch (reaction.type) {
    case QuestReactionType::ActivateQuest:
        reaction.target = reactionJson.at("quest").get<std::string>();
        break;
    case QuestReactionType::ActivateTrack:
        reaction.target = reactionJson.at("track").get<std::string>();
        break;
    case QuestReactionType::SetFlag:
        reaction.target = reactionJson.at("flag").get<std::string>();
        break;
    case QuestReactionType::SpawnEntity:
        reaction.target = reactionJson.value("entity", reactionJson.value("subject", ""));
        if (reaction.target.empty()) {
            throw std::runtime_error("SpawnEntity reaction requires entity or subject.");
        }
        reaction.position = Vec2(reactionJson.at("position"));
        break;
    case QuestReactionType::FinishStory:
        break;
    }

    return reaction;
}

void StoryManager::addQuest(const Quest& quest)
{
    if (m_quests.find(quest.id) == m_quests.end()) {
        m_questOrder.push_back(quest.id);
    }
    m_quests[quest.id] = quest;
}

int StoryManager::getCurrentQuestID()
{
    for (size_t i = 0; i < m_questOrder.size(); ++i) {
        const Quest& quest = m_quests.at(m_questOrder[i]);
        if (quest.track == MainTrack && quest.isActive()) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void StoryManager::setFlag(const std::string& flagName, bool value)
{
    m_storyFlags[flagName] = value;
}

void StoryManager::onEvent(const Event& e)
{
    if (e.type == EventType::EntityPossessed) {
        m_lastPossessedHost = e.itemName;
        m_currentHost = e.itemName;
    }

    std::vector<std::string> activeQuestIds;
    for (const std::string& questId : m_questOrder) {
        const Quest& quest = m_quests.at(questId);
        if (quest.isActive()) {
            activeQuestIds.push_back(questId);
        }
    }

    for (const std::string& questId : activeQuestIds) {
        Quest& quest = m_quests.at(questId);
        QuestStep* step = nullptr;
        if (quest.currentStep >= 0 && quest.currentStep < static_cast<int>(quest.steps.size())) {
            step = &quest.steps[quest.currentStep];
        }

        if (!step || !step->matches(e)) {
            continue;
        }

        const std::vector<QuestReaction> stepReactions = step->reactions;
        if (!quest.tryAdvance(e)) {
            continue;
        }

        for (const QuestReaction& reaction : stepReactions) {
            applyReaction(reaction);
        }

        if (quest.isCompleted()) {
            for (const QuestReaction& reaction : quest.reactions) {
                applyReaction(reaction);
            }
        }
    }
}

void StoryManager::applyReaction(const QuestReaction& reaction)
{
    switch (reaction.type) {
    case QuestReactionType::ActivateQuest:
        if (auto it = m_quests.find(reaction.target); it != m_quests.end() &&
            it->second.state == QuestState::Locked) {
            it->second.state = QuestState::Active;
        }
        break;
    case QuestReactionType::ActivateTrack:
        for (auto& [questId, quest] : m_quests) {
            (void)questId;
            if (quest.track == reaction.target && quest.state == QuestState::Locked) {
                quest.state = QuestState::Active;
            }
        }
        break;
    case QuestReactionType::SetFlag:
        setFlag(reaction.target, true);
        break;
    case QuestReactionType::SpawnEntity:
        if (m_spawnHandler) {
            m_spawnHandler(reaction.target, reaction.position);
        }
        break;
    case QuestReactionType::FinishStory:
        m_storyFinished = true;
        break;
    }
}

EventType StoryManager::getEventTypeFromString(const std::string& typeStr) const {
    if (typeStr == "ItemPickedUp") return EventType::ItemPickedUp;
    if (typeStr == "EntityKilled") return EventType::EntityKilled;
    if (typeStr == "EntitySpawned") return EventType::EntitySpawned;
    if (typeStr == "EntityDrained") return EventType::EntityDrained;
    if (typeStr == "EntityPossessed") return EventType::EntityPossessed;
    if (typeStr == "DialogueFinished") return EventType::DialogueFinished;
    if (typeStr == "FlagChanged") return EventType::FlagChanged;
    if (typeStr == "EnteredArea") return EventType::EnteredArea;
    throw std::runtime_error("Unknown event type: " + typeStr);
}

std::string StoryManager::getEventTypeName(EventType type) const
{
    switch (type) {
    case EventType::ItemPickedUp: return "ItemPickedUp";
    case EventType::EnteredArea: return "EnteredArea";
    case EventType::EntityKilled: return "EntityKilled";
    case EventType::EntitySpawned: return "EntitySpawned";
    case EventType::EntityDrained: return "EntityDrained";
    case EventType::EntityPossessed: return "EntityPossessed";
    case EventType::DialogueFinished: return "DialogueFinished";
    case EventType::FlagChanged: return "FlagChanged";
    case EventType::NoEvent: return "NoEvent";
    }
    return "NoEvent";
}

QuestState StoryManager::getQuestStateFromString(const std::string& stateStr) const
{
    if (stateStr == "locked") return QuestState::Locked;
    if (stateStr == "active") return QuestState::Active;
    if (stateStr == "completed") return QuestState::Completed;
    throw std::runtime_error("Unknown quest state: " + stateStr);
}

std::string StoryManager::getQuestStateName(QuestState state) const
{
    switch (state) {
    case QuestState::Locked: return "locked";
    case QuestState::Active: return "active";
    case QuestState::Completed: return "completed";
    }
    return "locked";
}

QuestReactionType StoryManager::getReactionTypeFromString(const std::string& typeStr) const
{
    if (typeStr == "ActivateQuest") return QuestReactionType::ActivateQuest;
    if (typeStr == "ActivateTrack") return QuestReactionType::ActivateTrack;
    if (typeStr == "SetFlag") return QuestReactionType::SetFlag;
    if (typeStr == "SpawnEntity") return QuestReactionType::SpawnEntity;
    if (typeStr == "FinishStory") return QuestReactionType::FinishStory;
    throw std::runtime_error("Unknown quest reaction type: " + typeStr);
}

bool StoryManager::isStoryFinished(){
    return m_storyFinished;
}

const Quest* StoryManager::getQuest(const std::string& questId) const
{
    auto it = m_quests.find(questId);
    if (it == m_quests.end()) {
        return nullptr;
    }
    return &it->second;
}

std::vector<const Quest*> StoryManager::activeQuests() const
{
    std::vector<const Quest*> quests;
    for (const std::string& questId : m_questOrder) {
        const Quest& quest = m_quests.at(questId);
        if (quest.isActive()) {
            quests.push_back(&quest);
        }
    }
    return quests;
}

std::string StoryManager::mainObjective() const
{
    for (const std::string& questId : m_questOrder) {
        const Quest& quest = m_quests.at(questId);
        const QuestStep* step = quest.activeStep();
        if (quest.track == MainTrack && step) {
            return step->text;
        }
    }
    return "";
}

std::vector<std::string> StoryManager::debugObjectiveLines() const
{
    std::vector<std::string> lines;

    const std::string mainText = mainObjective();
    if (!mainText.empty()) {
        lines.push_back("Main: " + mainText);
    }

    if (!m_currentHost.empty()) {
        lines.push_back("Host: " + m_currentHost);
    }

    for (const std::string& questId : m_questOrder) {
        const Quest& quest = m_quests.at(questId);
        const QuestStep* step = quest.activeStep();
        if (!step || quest.track == MainTrack) {
            continue;
        }
        lines.push_back(quest.title + ": " + step->text);
    }

    return lines;
}

json StoryManager::progressionJson() const
{
    json quests = json::object();
    for (const std::string& questId : m_questOrder) {
        const Quest& quest = m_quests.at(questId);
        quests[quest.id] = {
            {"state", getQuestStateName(quest.state)},
            {"currentStep", quest.currentStep}
        };
    }

    json flags = json::object();
    for (const auto& [flag, value] : m_storyFlags) {
        flags[flag] = value;
    }

    return {
        {"quests", quests},
        {"flags", flags},
        {"storyFinished", m_storyFinished},
        {"currentHost", m_currentHost},
        {"lastPossessedHost", m_lastPossessedHost}
    };
}

void StoryManager::loadProgression(const json& progression)
{
    if (progression.is_null() || !progression.is_object()) {
        return;
    }

    if (progression.contains("quests")) {
        for (const auto& [questId, questProgress] : progression.at("quests").items()) {
            auto it = m_quests.find(questId);
            if (it == m_quests.end()) {
                continue;
            }
            Quest& quest = it->second;
            quest.state = getQuestStateFromString(questProgress.value("state", getQuestStateName(quest.state)));
            quest.currentStep = questProgress.value("currentStep", quest.currentStep);
            if (quest.currentStep < 0) {
                quest.currentStep = 0;
            }
            if (quest.currentStep > static_cast<int>(quest.steps.size())) {
                quest.currentStep = static_cast<int>(quest.steps.size());
            }
            quest.restoreStepCompletions();
        }
    }

    if (progression.contains("flags")) {
        for (const auto& [flag, value] : progression.at("flags").items()) {
            m_storyFlags[flag] = value.get<bool>();
        }
    }

    m_storyFinished = progression.value("storyFinished", m_storyFinished);
    m_currentHost = progression.value("currentHost", m_currentHost);
    m_lastPossessedHost = progression.value("lastPossessedHost", m_lastPossessedHost);
}

void StoryManager::setSpawnHandler(std::function<void(const std::string&, Vec2)> handler)
{
    m_spawnHandler = std::move(handler);
}

void StoryManager::loadDialogs(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open dialogs file: " << path << "\n";
        return;
    }

    json data;
    file >> data;

    for (auto& [npcId, dialogJson] : data.at("dialogs").items()) {
        std::unordered_map<std::string, std::string> questDialogMap;

        if (dialogJson.is_object()) {
            for (auto& [questId, line] : dialogJson.items()) {
                questDialogMap[questId] = line.get<std::string>();
            }
        }
        else if (dialogJson.is_array()) {
            for (auto& obj : dialogJson) {
                for (auto& [questId, line] : obj.items()) {
                    questDialogMap[questId] = line.get<std::string>();
                }
            }
        }

        npcDialogs[npcId] = questDialogMap;
    }
}

const std::string& StoryManager::getDialog(const std::string& npcId) {
    static const std::string empty = "I have nothing to say right now...";

    auto npcIt = npcDialogs.find(npcId);
    if (npcIt == npcDialogs.end()) {
        return empty;
    }

    const dialogMap& dialogs = npcIt->second;

    for (const std::string& questId : m_questOrder) {
        const Quest& quest = m_quests.at(questId);
        const QuestStep* step = quest.activeStep();
        if (!step || quest.track == MainTrack || !step->trigger.involvesSubject(npcId)) {
            continue;
        }

        auto dialogIt = dialogs.find(quest.id);
        if (dialogIt != dialogs.end()) {
            return dialogIt->second;
        }
    }

    for (const std::string& questId : m_questOrder) {
        const Quest& quest = m_quests.at(questId);
        const QuestStep* step = quest.activeStep();
        if (!step || quest.track != MainTrack) {
            continue;
        }

        auto dialogIt = dialogs.find(quest.id);
        if (dialogIt != dialogs.end()) {
            return dialogIt->second;
        }
    }

    auto defaultIt = dialogs.find("default");
    if (defaultIt != dialogs.end()) {
        return defaultIt->second;
    }

    return empty;
}
