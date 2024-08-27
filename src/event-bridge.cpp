// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#include "event-bridge.hpp"

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
    EventBridge::Callback* const callback;
    std::vector<EventInput*> inputs;
    std::unordered_map<uint32_t, EventOutput*> outputs;

    Impl(EventBridge::Callback* const callback_, std::string& last_error_)
        : callback(callback_),
          last_error(last_error_) {}

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

    bool addInput(const EventInput::BackendType type, const char* const path)
    {
        if (EventInput* const input = EventInput::createNew(type, path))
        {
            inputs.push_back(input);
            return true;
        }

        return false;
    }

    bool addOutput(const EventOutput::BackendType type, const uint8_t index)
    {
        const uint32_t id = event_id(kEventTypeLED, index);

        if (EventOutput* const output = EventOutput::createNew(type, index))
        {
            outputs[id] = output;
            return true;
        }

        return false;
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
        if (callback != nullptr)
            callback->eventReceived(etype, index, value);
    }
};

// --------------------------------------------------------------------------------------------------------------------

EventBridge::EventBridge(Callback* const callback)
    : impl(new Impl(callback, last_error)) {}

EventBridge::~EventBridge() { delete impl; }

bool EventBridge::addInput(const EventInput::BackendType type, const char* const path)
{
    return impl->addInput(type, path);
}

bool EventBridge::addOutput(const EventOutput::BackendType type, const uint8_t index)
{
    return impl->addOutput(type, index);
}

void EventBridge::poll()
{
    impl->poll();
}

bool EventBridge::sendEvent(const EventType etype, const uint8_t index, const int16_t value)
{
    return impl->sendEvent(etype, index, value);
}

// --------------------------------------------------------------------------------------------------------------------
