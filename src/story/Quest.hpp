#pragma once

#include "story/EventBus.hpp"

#include <string>
#include <vector>

enum class QuestState {
    Locked,
    Active,
    Completed,
};

struct EventMatcher {
    EventType type = EventType::NoEvent;
    std::vector<std::string> subjects;

    bool matches(const Event& event) const;
};

struct QuestAction {
    std::string type;
    std::string questID;
};

struct TriggerReaction {
    EventMatcher trigger;
    std::vector<QuestAction> actions;
};

struct QuestStep {
    std::string id;
    std::string description;
    std::vector<EventMatcher> triggers;
    std::vector<TriggerReaction> reactionsByTrigger;
    bool completed = false;

    bool matches(const Event& event) const;
};

struct Quest {
    std::string id;
    std::string description;
    QuestState state = QuestState::Locked;
    std::vector<QuestStep> steps;
    size_t currentStep = 0;
    std::vector<QuestAction> reactions;
};
