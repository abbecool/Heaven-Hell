#pragma once

#include "physics/Vec2.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

using EntityID = uint32_t;

enum class EventType {
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

struct Event {
    EventType type = EventType::NoEvent;
    std::string itemName;
    Vec2 eventPosition = {-1, -1};
};

class EventBus {
public:
    using Listener = std::function<void(const Event&)>;

    void subscribe(const Event& event, const Listener& listener) {
        m_listenerMap[event.itemName].push_back(listener);
    }

    void emit(const Event& event) {
        const auto it = m_listenerMap.find(event.itemName);
        if (it == m_listenerMap.end()) {
            return;
        }

        for (const auto& listener : it->second) {
            listener(event);
        }
    }

private:
    std::unordered_map<std::string, std::vector<Listener>> m_listenerMap;
};
