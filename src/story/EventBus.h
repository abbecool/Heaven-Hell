// Event.h
#pragma once
#include <functional>
#include <vector>
#include <string>
#include <cstdint>   // For uint32_t (EntityID)

using EntityID = uint32_t;

enum class EventType {
    ItemPickedUp,
    EntityKilled,
    EntitySpawned,
    DialogueFinished,
    FlagChanged,
    NoEvent,
};

struct Event {
    EventType type = EventType::NoEvent;
    std::string itemName = "";       // optional
    Vec2 eventPosition = {-1, -1};         // optional
};

// Simple bus
class EventBus {
public:
    using Listener = std::function<void(const Event&)>;

    void subscribe(const Listener& listener) {
        m_listeners.push_back(listener);
    }

    void emit(const Event& e) {
        for (auto& l : m_listeners) {
            l(e);
        }
    }

private:
    std::vector<Listener> m_listeners;
};
