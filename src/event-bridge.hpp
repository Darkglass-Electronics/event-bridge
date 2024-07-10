// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#pragma once

#include <cstdint>
#include <string>

/**
 * The possible event types, for both receiving and sending.
 */
enum EventType {
    /**
     * Null event type.
     */
    kEventTypeNull = 0,

    /**
     * Encoder event type, an endless rotation actuator not bound to a minimum/maximum range.
     *
     * Positive values mean clock-wise rotation,
     * negative values mean anti-clock-wise rotation,
     * and 0 means "click".
     */
    kEventTypeEncoder,

    /**
     * Footswitch event type.
     * Works in "latched" mode, where a value of 1 means pressed and 0 means released.
     */
    kEventTypeFootswitch,

    /**
     * TBD.
     */
    kEventTypeLED,
};

/**
 * Convenience function to convert an event type to a string.
 */
static constexpr inline
const char* EventTypeStr(const EventType etype)
{
    switch (etype)
    {
    case kEventTypeNull:
        return "kEventTypeNull";
    case kEventTypeEncoder:
        return "kEventTypeEncoder";
    case kEventTypeFootswitch:
        return "kEventTypeFootswitch";
    case kEventTypeLED:
        return "kEventTypeLED";
    }
    return "";
}

/**
 * TODO document me
 */
struct EventBridge {
    struct Callbacks {
        virtual ~Callbacks() {}
        virtual void eventReceived(EventType etype, uint8_t index, int16_t value) = 0;
    };

   /**
    * string describing the last error, in case any operation fails.
    */
    std::string last_error;

    EventBridge(Callbacks* callbacks);
    ~EventBridge();

    void poll();
    bool sendEvent(EventType etype, uint8_t index, int16_t value);

private:
    struct Impl;
    Impl* const impl;
};
