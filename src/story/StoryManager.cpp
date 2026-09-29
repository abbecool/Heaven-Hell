#include "story/StoryManager.hpp"

#include "external/json.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unordered_set>

using json = nlohmann::json;

namespace
{

constexpr const char* StoryFormat = "story-quests-v1";

QuestState questStateFromString(const std::string& value)
{
    if (value == "locked")
        return QuestState::Locked;
    if (value == "active")
        return QuestState::Active;
    if (value == "completed")
        return QuestState::Completed;
    throw std::runtime_error("Unknown quest state: " + value);
}

std::vector<QuestAction>
parseActions(const json& object, const char* singularKey, const char* pluralKey)
{
    std::vector<QuestAction> actions;
    const auto addAction = [&actions](const json& action)
    {
        if (!action.is_object() || !action.contains("type"))
        {
            throw std::runtime_error("Story reaction must contain a type");
        }

        const std::string type = action.at("type").get<std::string>();
        if (type != "ActivateQuest")
        {
            throw std::runtime_error("Unknown story reaction type: " + type);
        }
        if (!action.contains("quest") || !action.at("quest").is_string())
        {
            throw std::runtime_error(
                "ActivateQuest reaction must contain a quest ID");
        }
        actions.push_back(
            QuestAction{type, action.at("quest").get<std::string>()});
    };

    if (object.contains(singularKey))
    {
        addAction(object.at(singularKey));
    }
    if (object.contains(pluralKey))
    {
        const json& actionArray = object.at(pluralKey);
        if (!actionArray.is_array())
        {
            throw std::runtime_error(std::string(pluralKey) +
                                     " must be an array");
        }
        for (const auto& action : actionArray)
        {
            addAction(action);
        }
    }
    return actions;
}

} // namespace

bool EventMatcher::matches(const Event& event) const
{
    return event.type == type && std::find(subjects.begin(), subjects.end(),
                                           event.itemName) != subjects.end();
}

bool QuestStep::matches(const Event& event) const
{
    return std::any_of(triggers.begin(), triggers.end(),
                       [&event](const EventMatcher& trigger)
                       { return trigger.matches(event); });
}

StoryManager::StoryManager(const std::string& storyFilePath)
{
    loadStory(storyFilePath);
    loadDialogs("config_files/dialogs.json");
}

void StoryManager::loadStory(const std::string& storyFilePath)
{
    std::ifstream file(storyFilePath);
    if (!file)
    {
        throw std::runtime_error("Could not load story file: " + storyFilePath);
    }

    json document;
    file >> document;
    if (!document.is_object() || document.value("format", "") != StoryFormat ||
        document.value("version", 0) != 1 || !document.contains("quests") ||
        !document.at("quests").is_array())
    {
        throw std::runtime_error(
            "Invalid Story1 document: expected format story-quests-v1, version "
            "1, and quests array");
    }

    uint64_t fingerprint = 14695981039346656037ULL;
    for (unsigned char byte : document.dump())
    {
        fingerprint ^= byte;
        fingerprint *= 1099511628211ULL;
    }
    m_definitionFingerprint = std::to_string(fingerprint);

    m_quests.clear();
    m_questIndices.clear();
    m_eventHistory.clear();
    m_primaryActiveQuestID.clear();
    m_storyFinished = false;

    size_t activeQuestCount = 0;
    for (const auto& questJson : document.at("quests"))
    {
        if (!questJson.is_object())
        {
            throw std::runtime_error("Story quest must be an object");
        }

        Quest quest;
        if (!questJson.contains("id") || !questJson.at("id").is_string() ||
            !questJson.contains("description") ||
            !questJson.at("description").is_string() ||
            !questJson.contains("initial_state") ||
            !questJson.at("initial_state").is_string() ||
            !questJson.contains("steps") || !questJson.at("steps").is_array() ||
            questJson.at("steps").empty())
        {
            throw std::runtime_error("Story quest must have id, description, "
                                     "initial_state, and non-empty steps");
        }

        quest.id = questJson.at("id").get<std::string>();
        if (!m_questIndices.emplace(quest.id, m_quests.size()).second)
        {
            throw std::runtime_error("Duplicate story quest ID: " + quest.id);
        }
        quest.description = questJson.at("description").get<std::string>();
        quest.state = questStateFromString(
            questJson.at("initial_state").get<std::string>());
        if (quest.state == QuestState::Active)
        {
            ++activeQuestCount;
            m_primaryActiveQuestID = quest.id;
        }

        std::unordered_set<std::string> stepIDs;
        for (const auto& stepJson : questJson.at("steps"))
        {
            if (!stepJson.is_object() || !stepJson.contains("id") ||
                !stepJson.at("id").is_string() ||
                !stepJson.contains("description") ||
                !stepJson.at("description").is_string() ||
                !stepJson.contains("triggers") ||
                !stepJson.at("triggers").is_array() ||
                stepJson.at("triggers").empty())
            {
                throw std::runtime_error("Story step must have id, "
                                         "description, and non-empty triggers");
            }

            QuestStep step;
            step.id = stepJson.at("id").get<std::string>();
            if (!stepIDs.insert(step.id).second)
            {
                throw std::runtime_error("Duplicate step ID in quest " +
                                         quest.id + ": " + step.id);
            }
            step.description = stepJson.at("description").get<std::string>();

            const auto parseMatcher = [this](const json& trigger)
            {
                if (!trigger.is_object() || !trigger.contains("event_type") ||
                    !trigger.at("event_type").is_string() ||
                    !trigger.contains("subjects") ||
                    !trigger.at("subjects").is_array() ||
                    trigger.at("subjects").empty())
                {
                    throw std::runtime_error(
                        "Story trigger must have event_type and non-empty "
                        "subjects");
                }

                EventMatcher matcher;
                matcher.type = getEventTypeFromString(
                    trigger.at("event_type").get<std::string>());
                for (const auto& subject : trigger.at("subjects"))
                {
                    if (!subject.is_string())
                    {
                        throw std::runtime_error(
                            "Story trigger subjects must be strings");
                    }
                    matcher.subjects.push_back(subject.get<std::string>());
                }
                return matcher;
            };

            for (const auto& triggerJson : stepJson.at("triggers"))
            {
                step.triggers.push_back(parseMatcher(triggerJson));
            }
            if (stepJson.contains("reactions_by_trigger"))
            {
                const json& reactionRules = stepJson.at("reactions_by_trigger");
                if (!reactionRules.is_array())
                {
                    throw std::runtime_error(
                        "reactions_by_trigger must be an array");
                }
                for (const auto& reactionRule : reactionRules)
                {
                    if (!reactionRule.is_object() ||
                        !reactionRule.contains("trigger"))
                    {
                        throw std::runtime_error(
                            "Trigger reaction must contain a trigger");
                    }
                    TriggerReaction triggerReaction;
                    triggerReaction.trigger =
                        parseMatcher(reactionRule.at("trigger"));
                    triggerReaction.actions =
                        parseActions(reactionRule, "reaction", "reactions");
                    if (triggerReaction.actions.empty())
                    {
                        throw std::runtime_error(
                            "Trigger reaction must contain an action");
                    }
                    step.reactionsByTrigger.push_back(
                        std::move(triggerReaction));
                }
            }
            quest.steps.push_back(std::move(step));
        }
        quest.reactions = parseActions(questJson, "reaction", "reactions");
        m_quests.push_back(std::move(quest));
    }

    if (m_quests.empty())
    {
        throw std::runtime_error(
            "Story document must contain at least one quest");
    }
    if (activeQuestCount != 1)
    {
        throw std::runtime_error(
            "Story document must contain exactly one initially active quest");
    }
    for (const Quest& quest : m_quests)
    {
        for (const QuestAction& action : quest.reactions)
        {
            questByID(action.questID);
        }
        for (const QuestStep& step : quest.steps)
        {
            for (const TriggerReaction& reaction : step.reactionsByTrigger)
            {
                for (const QuestAction& action : reaction.actions)
                {
                    questByID(action.questID);
                }
            }
        }
    }
}

void StoryManager::onEvent(const Event& event)
{
    // Area contacts occur each frame. Only discrete actions may satisfy a
    // quest which becomes active after the action took place.
    if (!recordsPriorAction(event.type))
    {
        m_eventHistory.push_back(RecordedEvent{event, false});
        processRecordedEvent(m_eventHistory.size() - 1);
        m_eventHistory.pop_back();
        return;
    }

    const size_t index = m_eventHistory.size();
    m_eventHistory.push_back(RecordedEvent{event, false});
    processRecordedEvent(index);
    // Keep at most one unmatched occurrence of each discrete action/subject.
    // A bounded history also prevents long sessions from growing without limit.
    if (!m_eventHistory[index].consumed)
    {
        for (size_t i = 0; i < index; ++i)
        {
            if (!m_eventHistory[i].consumed &&
                m_eventHistory[i].event.type == event.type &&
                m_eventHistory[i].event.itemName == event.itemName)
            {
                m_eventHistory.pop_back();
                break;
            }
        }
    }
    if (m_eventHistory.size() > 128)
    {
        m_eventHistory.erase(m_eventHistory.begin());
    }
}

bool StoryManager::recordsPriorAction(EventType type)
{
    switch (type)
    {
    case EventType::ItemPickedUp:
    case EventType::EntityKilled:
    case EventType::EntityDrained:
    case EventType::EntityPossessed:
    case EventType::DialogueFinished:
    case EventType::FlagChanged:
        return true;
    default:
        return false;
    }
}

json StoryManager::saveState() const
{
    json quests = json::array();
    for (const Quest& quest : m_quests)
    {
        json stepIDs = json::array();
        for (const QuestStep& step : quest.steps)
            stepIDs.push_back(step.id);
        const char* state = quest.state == QuestState::Locked ? "locked" :
                            quest.state == QuestState::Active ? "active" : "completed";
        quests.push_back({{"id", quest.id}, {"steps", stepIDs},
                          {"state", state}, {"current_step", quest.currentStep}});
    }

    json priorActions = json::array();
    for (const RecordedEvent& record : m_eventHistory)
    {
        if (!record.consumed && recordsPriorAction(record.event.type))
        {
            priorActions.push_back({{"type", static_cast<int>(record.event.type)},
                                    {"subject", record.event.itemName}});
        }
    }
    return {{"version", 1}, {"definition", m_definitionFingerprint},
            {"quests", quests},
            {"prior_actions", priorActions}, {"finished", m_storyFinished}};
}

void StoryManager::loadState(const json& state)
{
    if (!state.is_object() || state.at("version") != 1 ||
        state.at("definition") != m_definitionFingerprint ||
        !state.at("quests").is_array() ||
        state.at("quests").size() != m_quests.size() ||
        !state.at("prior_actions").is_array() ||
        state.at("prior_actions").size() > 128)
    {
        throw std::runtime_error("Incompatible quest save state");
    }

    std::vector<Quest> restored = m_quests;
    size_t activeCount = 0;
    for (size_t i = 0; i < restored.size(); ++i)
    {
        const json& entry = state.at("quests").at(i);
        Quest& quest = restored[i];
        if (entry.at("id") != quest.id || !entry.at("steps").is_array() ||
            entry.at("steps").size() != quest.steps.size())
            throw std::runtime_error("Quest definitions changed since save");
        for (size_t s = 0; s < quest.steps.size(); ++s)
        {
            if (entry.at("steps").at(s) != quest.steps[s].id)
                throw std::runtime_error("Quest steps changed since save");
        }
        quest.state = questStateFromString(entry.at("state").get<std::string>());
        quest.currentStep = entry.at("current_step").get<size_t>();
        if (quest.currentStep > quest.steps.size() ||
            (quest.state == QuestState::Completed) !=
                (quest.currentStep == quest.steps.size()) ||
            (quest.state == QuestState::Locked && quest.currentStep != 0))
            throw std::runtime_error("Invalid saved quest progression");
        for (size_t s = 0; s < quest.steps.size(); ++s)
            quest.steps[s].completed = s < quest.currentStep;
        activeCount += quest.state == QuestState::Active ? 1 : 0;
    }
    const bool finished = state.at("finished").get<bool>();
    if (activeCount > 1 || (finished != (activeCount == 0)))
        throw std::runtime_error("Invalid saved active quest state");

    std::vector<RecordedEvent> history;
    for (const json& action : state.at("prior_actions"))
    {
        int rawType = action.at("type").get<int>();
        if (rawType < 0 || rawType >= static_cast<int>(EventType::NoEvent) ||
            !recordsPriorAction(static_cast<EventType>(rawType)))
            throw std::runtime_error("Invalid saved prior action");
        Event event{static_cast<EventType>(rawType),
                    action.at("subject").get<std::string>()};
        const auto duplicate = std::find_if(history.begin(), history.end(),
            [&event](const RecordedEvent& previous) {
                return previous.event.type == event.type &&
                       previous.event.itemName == event.itemName;
            });
        if (duplicate != history.end())
            throw std::runtime_error("Duplicate saved prior action");
        history.push_back({event, false});
    }
    m_quests = std::move(restored);
    m_eventHistory = std::move(history);
    m_storyFinished = finished;
    refreshPrimaryActiveQuest();
}

bool StoryManager::isStoryFinished() const
{
    return m_storyFinished;
}

QuestState StoryManager::getQuestState(const std::string& questID) const
{
    return questByID(questID).state;
}

bool StoryManager::isQuestActive(const std::string& questID) const
{
    return getQuestState(questID) == QuestState::Active;
}

const std::string& StoryManager::getPrimaryActiveQuestID() const
{
    return m_primaryActiveQuestID;
}

const std::vector<Quest>& StoryManager::getQuests() const
{
    return m_quests;
}

EventType StoryManager::getEventTypeFromString(const std::string& typeStr) const
{
    if (typeStr == "ItemPickedUp")
        return EventType::ItemPickedUp;
    if (typeStr == "EntityKilled")
        return EventType::EntityKilled;
    if (typeStr == "EntitySpawned")
        return EventType::EntitySpawned;
    if (typeStr == "DialogueFinished")
        return EventType::DialogueFinished;
    if (typeStr == "FlagChanged")
        return EventType::FlagChanged;
    if (typeStr == "EnteredArea")
        return EventType::EnteredArea;
    if (typeStr == "EntityDrained")
        return EventType::EntityDrained;
    if (typeStr == "EntityPossessed")
        return EventType::EntityPossessed;
    throw std::runtime_error("Unknown story event type: " + typeStr);
}

void StoryManager::loadDialogs(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
    {
        std::cerr << "Failed to open dialogs file: " << path << '\n';
        return;
    }

    json document;
    file >> document;
    if (!document.contains("dialogs") || !document.at("dialogs").is_object())
    {
        std::cerr << "Invalid dialogs file: " << path << '\n';
        return;
    }

    m_npcDialogs.clear();
    for (const auto& [npcID, dialogArray] : document.at("dialogs").items())
    {
        DialogMap questDialogMap;
        if (!dialogArray.is_array())
        {
            continue;
        }
        for (const auto& dialog : dialogArray)
        {
            if (!dialog.is_object())
            {
                continue;
            }
            for (const auto& [questID, line] : dialog.items())
            {
                if (line.is_string())
                {
                    questDialogMap[questID] = line.get<std::string>();
                }
            }
        }
        m_npcDialogs[npcID] = std::move(questDialogMap);
    }
}

const std::string& StoryManager::getDialog(const std::string& npcID) const
{
    static const std::string fallback = "I have nothing to say right now...";
    const auto npc = m_npcDialogs.find(npcID);
    if (npc == m_npcDialogs.end() || m_primaryActiveQuestID.empty())
    {
        return fallback;
    }
    const auto dialog = npc->second.find(m_primaryActiveQuestID);
    return dialog == npc->second.end() ? fallback : dialog->second;
}

Quest& StoryManager::questByID(const std::string& questID)
{
    const auto it = m_questIndices.find(questID);
    if (it == m_questIndices.end())
    {
        throw std::runtime_error("Unknown story quest ID: " + questID);
    }
    return m_quests.at(it->second);
}

const Quest& StoryManager::questByID(const std::string& questID) const
{
    const auto it = m_questIndices.find(questID);
    if (it == m_questIndices.end())
    {
        throw std::runtime_error("Unknown story quest ID: " + questID);
    }
    return m_quests.at(it->second);
}

void StoryManager::activateQuest(const std::string& questID)
{
    Quest& quest = questByID(questID);
    if (quest.state != QuestState::Locked)
    {
        return;
    }

    quest.state = QuestState::Active;
    refreshPrimaryActiveQuest();
    std::cout << "New quest: " << quest.description << '\n';
    replayHistoricalEvents();
}

void StoryManager::replayHistoricalEvents()
{
    for (size_t index = 0; index < m_eventHistory.size(); ++index)
    {
        if (!m_eventHistory[index].consumed && processRecordedEvent(index))
        {
            break;
        }
    }
}

bool StoryManager::processRecordedEvent(size_t eventIndex)
{
    if (eventIndex >= m_eventHistory.size() ||
        m_eventHistory[eventIndex].consumed)
    {
        return false;
    }

    const Event event = m_eventHistory[eventIndex].event;
    std::vector<std::string> activeQuestIDs;
    for (const Quest& quest : m_quests)
    {
        if (quest.state == QuestState::Active)
        {
            activeQuestIDs.push_back(quest.id);
        }
    }

    for (const std::string& questID : activeQuestIDs)
    {
        Quest& quest = questByID(questID);
        if (quest.state != QuestState::Active ||
            quest.currentStep >= quest.steps.size() ||
            !quest.steps[quest.currentStep].matches(event))
        {
            continue;
        }
        m_eventHistory[eventIndex].consumed = true;
        return advanceQuest(quest, event);
    }
    return false;
}

bool StoryManager::advanceQuest(Quest& quest, const Event& event)
{
    if (quest.currentStep >= quest.steps.size())
    {
        return false;
    }

    QuestStep& step = quest.steps[quest.currentStep];
    if (!step.matches(event))
    {
        return false;
    }

    std::vector<QuestAction> triggerActions;
    for (const TriggerReaction& reaction : step.reactionsByTrigger)
    {
        if (reaction.trigger.matches(event))
        {
            triggerActions = reaction.actions;
            break;
        }
    }

    step.completed = true;
    ++quest.currentStep;
    const bool questCompleted = quest.currentStep == quest.steps.size();
    if (questCompleted)
    {
        quest.state = QuestState::Completed;
    }

    executeActions(triggerActions);
    if (questCompleted)
    {
        executeActions(quest.reactions);
    }
    refreshPrimaryActiveQuest();
    if (questCompleted && m_primaryActiveQuestID.empty())
    {
        m_storyFinished = true;
    }
    return true;
}

void StoryManager::executeActions(const std::vector<QuestAction>& actions)
{
    for (const QuestAction& action : actions)
    {
        if (action.type == "ActivateQuest")
        {
            activateQuest(action.questID);
        }
    }
}

void StoryManager::refreshPrimaryActiveQuest()
{
    m_primaryActiveQuestID.clear();
    for (const Quest& quest : m_quests)
    {
        if (quest.state == QuestState::Active)
        {
            m_primaryActiveQuestID = quest.id;
            return;
        }
    }
}
