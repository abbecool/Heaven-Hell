#include "../story/StoryManager.h"

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
    else //if ( m_progression >= 0 )
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
