// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#include "event-bridge.hpp"
#include "events.hpp"

#include <unordered_map>
#include <vector>

// --------------------------------------------------------------------------------------------------------------------

static inline constexpr uint32_t event_id(const EventType etype, const uint8_t index) noexcept
{
    return static_cast<uint32_t>(etype) * UINT8_MAX + index;
}

// --------------------------------------------------------------------------------------------------------------------

struct EventBridge::Impl : EventInput::Callback
{
    Callbacks* const callbacks;
    std::vector<EventInput*> inputs;
    std::unordered_map<uint32_t, EventOutput*> outputs;

    Impl(Callbacks* const callbacks_, std::string& last_error_)
        : callbacks(callbacks_),
          last_error(last_error_)
    {
        if (EventInput* const input = EventInput::createNew(EventInput::kBackendTypeLibInput))
            inputs.push_back(input);
    }

    ~Impl()
    {
        for (EventInput* input : inputs)
            delete input;

        for (auto& item : outputs)
            delete item.second;

        close();
    }

    void close()
    {
    }

    void poll()
    {
        for (EventInput* input : inputs)
            input->poll(this);
    }

    bool sendEvent(const EventType etype, const uint8_t index, const int16_t value)
    {
        const uint32_t id = event_id(etype, index);

        for (auto& item : outputs)
        {
            if (item.first == id)
                item.second->event(value);
        }

        return true;
    }

private:
    std::string& last_error;

    void event(EventType etype, uint8_t index, int16_t value) override
    {
        callbacks->eventReceived(etype, index, value);
    }
};

// --------------------------------------------------------------------------------------------------------------------

EventBridge::EventBridge(Callbacks* const callbacks)
    : impl(new Impl(callbacks, last_error)) {}

EventBridge::~EventBridge() { delete impl; }

void EventBridge::poll()
{
    impl->poll();
}

bool EventBridge::sendEvent(const EventType etype, const uint8_t index, const int16_t value)
{
    return impl->sendEvent(etype, index, value);
}

// --------------------------------------------------------------------------------------------------------------------
