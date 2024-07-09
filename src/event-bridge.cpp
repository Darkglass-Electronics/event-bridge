// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#include "event-bridge.hpp"
#include "events.hpp"

#include <vector>

// --------------------------------------------------------------------------------------------------------------------

struct EventBridge::Impl : EventInput::Callback
{
    Callbacks* const callbacks;
    std::vector<EventInput*> inputs;

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

// --------------------------------------------------------------------------------------------------------------------
