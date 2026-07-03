// Event.h
#pragma once
#include <functional>
#include <vector>
#include <string>
#include <cstdint>   // For uint32_t (EntityID)

using EntityID = uint32_t;

enum class EventType {
    ItemPickedUp,
    EnteredArea,
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
    EventBus(){};
    using Listener = std::function<void(const Event&)>;

    void subscribe(Event e, const Listener& listener) {
        std::vector<Listener>& v = m_listenerMap[e.itemName];
        v.push_back(listener);
    }

    void emit(const Event& e) {
        if (m_listenerMap.find(e.itemName) == m_listenerMap.end()){
            return;
        }
        
        auto listeners = m_listenerMap.at(e.itemName);
        for (auto& l : listeners) {
            l(e);
        }
    }

private:
    std::vector<Listener> m_listeners;
    std::unordered_map<std::string, std::vector<Listener>> m_listenerMap;
};
