#pragma once
#include <string>
#include <iostream>
#include "external/json.hpp"
#include "physics/Vec2.h"
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
