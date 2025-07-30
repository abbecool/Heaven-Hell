#pragma once
#include "ecs/ECS.hpp"
#include "scenes/Scene.h"

#include <unordered_map>
#include <vector>
#include <string>

using Progression = uint8_t;

class StoryManager
{
    private:
    ECS* m_ECS;
    Scene* m_scene;
    Progression m_progression = 0;
    
    public:
    StoryManager() {};
    StoryManager(ECS* ecs, Scene* scene);
    std::string getDialog();
    Progression getProgression();
    void updateProgression();
};

class StoryManagerChat {
public:
    void setFlag(const std::string& flagName, bool value);
    bool getFlag(const std::string& flagName) const;

    void registerDialog(const std::string& npcId, const std::vector<std::string>& lines);
    const std::string& getDialog(const std::string& npcId);

private:
    std::unordered_map<std::string, bool> storyFlags;
    std::unordered_map<std::string, std::vector<std::string>> npcDialogs;
    std::unordered_map<std::string, int> npcTalkCounts;

    const std::string defaultDialog = "They have nothing to say.";
};
