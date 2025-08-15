#pragma once
#include <string>
#include <iostream>
#include "external/json.hpp"
#include "physics/Vec2.h"
using json = nlohmann::json;

struct Trigger
{
    std::string type;
    std::string name;
    std::string target;
    Trigger() {};
    Trigger(const json& j) {
        type = j["type"];
        name = j["name"];
    }
};

struct OnComplete
{
    std::string type;
    std::string name;
    bool value;
    OnComplete() {};
    OnComplete(const json& j) {
        type = j["type"];
        name = j["name"];
    }
};

struct StoryQuest
{
    int id;
    std::string description;

    std::string triggerType;
    std::string triggerName;
    bool        triggerValue = false;

    std::string onCompleteType;
    std::string onCompleteEntity;
    Vec2        onCompleteLocation;
    std::string onCompleteName;
    bool        onCompleteValue = false;

    StoryQuest() {};
    int getId() const { return id; }
    std::string getDescription() const { return description; }

    void setId(const int& i) { id = i; }
    void setDescription(const std::string& d) { description = d; }

    void setTriggerValue(bool value) { triggerValue = value; }
};
