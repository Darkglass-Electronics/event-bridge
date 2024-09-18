// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#include "event-bridge.hpp"

#include <unordered_map>
#include <vector>

// --------------------------------------------------------------------------------------------------------------------

static inline constexpr EventType event_type(const EventOutput::BackendType type) noexcept
{
    switch (type)
    {
    case EventOutput::kBackendTypeNull:
        return kEventTypeNull;
    case EventOutput::kBackendTypeGPIO:
    case EventOutput::kBackendTypeSysfsLED:
        return kEventTypeLED;
    }

    return kEventTypeNull;
}

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

    bool addInput(const EventInput::BackendType type, const char* const id, const uint8_t index)
    {
        if (EventInput* const input = EventInput::createNew(type, id, index))
        {
            inputs.push_back(input);
            return true;
        }

        return false;
    }

    bool addOutput(const EventOutput::BackendType type, const char* const id, const uint8_t index)
    {
        const uint32_t idx = event_id(event_type(type), index);

        if (EventOutput* const output = EventOutput::createNew(type, id))
        {
            outputs[idx] = output;
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
        const uint32_t idx = event_id(etype, index);

        for (auto& item : outputs)
        {
            if (item.first == idx)
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

bool EventBridge::addInput(const EventInput::BackendType type, const char* const id, const uint8_t index)
{
    return impl->addInput(type, id, index);
}

bool EventBridge::addOutput(const EventOutput::BackendType type, const char* const id, const uint8_t index)
{
    return impl->addOutput(type, id, index);
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
