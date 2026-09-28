#pragma once

#include "physics/Vec2.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

using EntityID = uint32_t;

enum class EventType
{
    ItemPickedUp,
    EnteredArea,
    EntityKilled,
    EntitySpawned,
    DialogueFinished,
    FlagChanged,
    EntityDrained,
    EntityPossessed,
    NoEvent,
};

struct Event
{
    EventType type = EventType::NoEvent;
    std::string itemName;
    Vec2 eventPosition = {-1, -1};
};

class EventBus
{
public:
    using Listener = std::function<void(const Event&)>;

    void subscribe(const Event& event, const Listener& listener)
    {
        m_listenerMap[{event.type, event.itemName}].push_back(listener);
    }

    void emit(const Event& event)
    {
        const auto it = m_listenerMap.find({event.type, event.itemName});
        if (it == m_listenerMap.end())
        {
            return;
        }

        for (const auto& listener : it->second)
        {
            listener(event);
        }
    }

private:
    struct Key
    {
        EventType type;
        std::string subject;

        bool operator==(const Key&) const = default;
    };

    struct KeyHash
    {
        size_t operator()(const Key& key) const
        {
            const size_t typeHash = std::hash<int>{}(static_cast<int>(key.type));
            return typeHash ^ (std::hash<std::string>{}(key.subject) << 1);
        }
    };

    std::unordered_map<Key, std::vector<Listener>, KeyHash> m_listenerMap;
};
