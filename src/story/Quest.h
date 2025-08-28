#pragma once
#include <string>
#include <iostream>
#include "external/json.hpp"
#include "physics/Vec2.h"
#include "story/EventBus.h"
using json = nlohmann::json;

struct QuestStep {
    EventType requiredType = EventType::NoEvent;
    std::string requiredSubject = ""; // e.g. "wizard", "staff"
    bool completed = false;

    bool matches(const Event& e) const {
        return e.type == requiredType && e.itemName == requiredSubject;
    }
    Event asEvent(){
        return Event{requiredType, requiredSubject};
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