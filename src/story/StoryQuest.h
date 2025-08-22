#pragma once
#include <string>
#include <iostream>
#include "external/json.hpp"
#include "physics/Vec2.h"
#include "story/EventBus.h"
using json = nlohmann::json;

struct StoryQuest
{
    int id;
    std::string description;

    std::string triggerType;
    std::string triggerName;
    bool        triggerValue = false;

    std::string onCompleteType;
    std::string onCompleteEntity;
    Vec2        onCompleteposition;
    std::string onCompleteName;
    bool        onCompleteValue = false;

    StoryQuest() {};

    int getId() const { return id; }
    std::string getDescription() const { return description; }

    void setId(const int& i) { id = i; }
    void setDescription(const std::string& d) { description = d; }
    void setTriggerValue(bool value) { triggerValue = value; }
};

struct QuestStep {
    EventType requiredType;
    std::string requiredSubject; // e.g. "wizard", "staff"
    bool completed = false;

    bool matches(const Event& e) const {
        return e.type == requiredType && e.itemName == requiredSubject;
    }
};

struct Quest {
    int id;
    std::string name;
    std::vector<QuestStep> steps;
    int currentStep = 0;

    // What happens on completion
    Event onComplete;

    void tryAdvance(const Event& e) {
        if (currentStep < steps.size() && steps[currentStep].matches(e)) {
            steps[currentStep].completed = true;
            currentStep++;
            // emit quest progress event
        }
    }
};