#pragma once
#include "story/Quest.hpp"
#include "external/json.hpp"

#include <functional>
#include <unordered_map>
#include <vector>
#include <string>

class Scene_Play;

using dialogMap = std::unordered_map<std::string, std::string>;
using NPCDialogs = std::unordered_map<std::string, dialogMap>;
using json = nlohmann::json;

class StoryManager
{
    private:
    bool m_storyFinished = false;
    std::unordered_map<std::string, Quest> m_quests;
    std::vector<std::string> m_questOrder;
    std::unordered_map<std::string, bool> m_storyFlags;
    std::string m_currentHost;
    std::string m_lastPossessedHost;
    std::function<void(const std::string&, Vec2)> m_spawnHandler;

    NPCDialogs npcDialogs;

    void loadQuestFile(const std::string& questFilePath);
    Quest parseQuest(const json& questJson) const;
    QuestStep parseStep(const json& stepJson) const;
    QuestTrigger parseTrigger(const json& triggerJson) const;
    QuestReaction parseReaction(const json& reactionJson) const;
    void addQuest(const Quest& quest);
    void applyReaction(const QuestReaction& reaction);
    std::string mainObjective() const;
    
    public:
    StoryManager() {};
    StoryManager(Scene_Play* scene, std::string storyFilePath, std::string questFilePath);

    void loadStory(const std::string& storyFilePath);
    void loadQuests(const std::string& questsFilePath);
    int getCurrentQuestID();
    void setFlag(const std::string& flagName, bool value);
    void onEvent(const Event& e);
    EventType getEventTypeFromString(const std::string& typeStr) const;
    std::string getEventTypeName(EventType type) const;
    QuestState getQuestStateFromString(const std::string& stateStr) const;
    std::string getQuestStateName(QuestState state) const;
    QuestReactionType getReactionTypeFromString(const std::string& typeStr) const;
    bool isStoryFinished();
    const Quest* getQuest(const std::string& questId) const;
    std::vector<const Quest*> activeQuests() const;
    std::vector<std::string> debugObjectiveLines() const;
    json progressionJson() const;
    void loadProgression(const json& progression);
    void setSpawnHandler(std::function<void(const std::string&, Vec2)> handler);
    void loadDialogs(const std::string& path);
    const std::string& getDialog(const std::string& npcId);
};
