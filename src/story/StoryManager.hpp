#pragma once

#include "story/Quest.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

using DialogMap = std::unordered_map<std::string, std::string>;
using NPCDialogs = std::unordered_map<std::string, DialogMap>;

class StoryManager {
public:
    StoryManager() = default;
    explicit StoryManager(const std::string& storyFilePath);

    void loadStory(const std::string& storyFilePath);
    void onEvent(const Event& event);
    EventType getEventTypeFromString(const std::string& typeStr) const;
    bool isStoryFinished() const;
    QuestState getQuestState(const std::string& questID) const;
    bool isQuestActive(const std::string& questID) const;
    const std::string& getPrimaryActiveQuestID() const;
    const std::vector<Quest>& getQuests() const;
    void loadDialogs(const std::string& path);
    const std::string& getDialog(const std::string& npcID) const;

private:
    struct RecordedEvent {
        Event event;
        bool consumed = false;
    };

    std::vector<Quest> m_quests;
    std::unordered_map<std::string, size_t> m_questIndices;
    std::vector<RecordedEvent> m_eventHistory;
    std::string m_primaryActiveQuestID;
    bool m_storyFinished = false;
    NPCDialogs m_npcDialogs;

    Quest& questByID(const std::string& questID);
    const Quest& questByID(const std::string& questID) const;
    void activateQuest(const std::string& questID);
    void replayHistoricalEvents();
    bool processRecordedEvent(size_t eventIndex);
    bool advanceQuest(Quest& quest, const Event& event);
    void executeActions(const std::vector<QuestAction>& actions);
    void refreshPrimaryActiveQuest();
};
