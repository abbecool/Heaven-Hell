#include "story/StoryManager.h"

StoryManager::StoryManager(ECS* ecs, Scene* scene)
{
    m_ECS = ecs;
    m_scene = scene;
}

std::string StoryManager::getDialog()
{
    std::string dialog;
    if ( m_progression == 0 )
    {
        dialog = "hello, traveler!: "+std::to_string(m_progression);
    }
    else
    {
        dialog = "hello again, traveler!: "+std::to_string(m_progression);
    }
    updateProgression();
    return dialog;
}


Progression StoryManager::getProgression()
{
    return m_progression;
}

void StoryManager::updateProgression()
{
    m_progression++;
}

void StoryManagerChat::setFlag(const std::string& flagName, bool value) {
    storyFlags[flagName] = value;
}

bool StoryManagerChat::getFlag(const std::string& flagName) const {
    auto it = storyFlags.find(flagName);
    return it != storyFlags.end() && it->second;
}

void StoryManagerChat::registerDialog(const std::string& npcId, const std::vector<std::string>& lines) {
    npcDialogs[npcId] = lines;
    npcTalkCounts[npcId] = 0;
}

const std::string& StoryManagerChat::getDialog(const std::string& npcId) {
    auto it = npcDialogs.find(npcId);
    if (it == npcDialogs.end()) return defaultDialog;

    int& count = npcTalkCounts[npcId];
    const auto& lines = it->second;

    const std::string& line = lines[std::min(count, static_cast<int>(lines.size() - 1))];
    count++;
    return line;
}

