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
};

struct Event {
    EventType type;
    std::string itemName;       // optional
    Vec2 eventPosition;         // optional
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
