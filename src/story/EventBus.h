// Event.h
#pragma once
#include <functional>
#include <vector>
#include <string>
#include <cstdint>   // For uint32_t (EntityID)

using EntityID = uint32_t;

enum class GameEventType {
    ItemPickedUp,
    EntityKilled,
    DialogueFinished,
    Custom
};

struct GameEvent {
    GameEventType type;
    EntityID entityId = -1;        // optional, -1 if none
    std::string itemName;     // optional
};

// Simple bus
class EventBus {
public:
    using Listener = std::function<void(const GameEvent&)>;

    void subscribe(const Listener& listener) {
        m_listeners.push_back(listener);
    }

    void emit(const GameEvent& e) {
        for (auto& l : m_listeners) {
            l(e);
        }
    }

private:
    std::vector<Listener> m_listeners;
};
