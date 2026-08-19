#pragma once
#include <string>
#include <vector>
#include "story/EventBus.hpp"

enum class QuestState {
    Locked,
    Active,
    Completed
};

enum class QuestReactionType {
    ActivateQuest,
    ActivateTrack,
    SetFlag,
    SpawnEntity,
    FinishStory
};

struct QuestTrigger {
    EventType eventType = EventType::NoEvent;
    std::vector<std::string> subjects;

    bool matches(const Event& e) const {
        if (e.type != eventType) {
            return false;
        }
        if (subjects.empty()) {
            return true;
        }
        for (const std::string& subject : subjects) {
            if (subject == e.itemName) {
                return true;
            }
        }
        return false;
    }

    bool involvesSubject(const std::string& subject) const {
        for (const std::string& requiredSubject : subjects) {
            if (requiredSubject == subject) {
                return true;
            }
        }
        return false;
    }
};

struct QuestReaction {
    QuestReactionType type = QuestReactionType::SetFlag;
    std::string target;
    Vec2 position = {-1, -1};
};

struct QuestStep {
    std::string id;
    std::string text;
    QuestTrigger trigger;
    std::vector<QuestReaction> reactions;
    bool completed = false;

    bool matches(const Event& e) const {
        return trigger.matches(e);
    }
};

struct Quest {
    std::string id;
    std::string title;
    std::string track;
    QuestState state = QuestState::Locked;
    std::vector<QuestStep> steps;
    int currentStep = 0;
    std::vector<QuestReaction> reactions;

    bool isActive() const {
        return state == QuestState::Active;
    }

    bool isCompleted() const {
        return state == QuestState::Completed;
    }

    const QuestStep* activeStep() const {
        if (state != QuestState::Active ||
            currentStep < 0 ||
            currentStep >= static_cast<int>(steps.size())) {
            return nullptr;
        }
        return &steps[currentStep];
    }

    bool tryAdvance(const Event& e) {
        if (state != QuestState::Active ||
            currentStep < 0 ||
            currentStep >= static_cast<int>(steps.size()) ||
            !steps[currentStep].matches(e)) {
            return false;
        }

        steps[currentStep].completed = true;
        currentStep++;
        if (currentStep >= static_cast<int>(steps.size())) {
            state = QuestState::Completed;
        }
        return true;
    }

    void restoreStepCompletions() {
        for (int i = 0; i < static_cast<int>(steps.size()); ++i) {
            steps[i].completed = i < currentStep || state == QuestState::Completed;
        }
    }
};
