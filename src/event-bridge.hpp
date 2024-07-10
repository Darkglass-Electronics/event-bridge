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
     * A value of 1 means pressed and 0 means released.
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
 * Event Bridge class that can both receive and send events.
 */
struct EventBridge {
    /**
     * Callback used for receiving events, triggered via poll().
     */
    struct Callback {
        /** destructor */
        virtual ~Callback() {}

        /**
         * Event trigger function, called when an event is received.
         */
        virtual void eventReceived(EventType etype, uint8_t index, int16_t value) = 0;
    };

   /**
    * string describing the last error, in case any operation fails.
    */
    std::string last_error;

    /** constructor, optionally passing a callback for receiving events */
    EventBridge(Callback* callback = nullptr);

    /** destructor */
    virtual ~EventBridge();

    /**
     * Event polling function, to be called at regular intervals.
     * Will trigger event received callbacks if there were any events during the last period.
     * @note this function is very likely to be replaced with an FD-based event polling later on.
     */
    void poll();

    /**
     * Event trigger function, to be called for sending events.
     */
    bool sendEvent(EventType etype, uint8_t index, int16_t value);

private:
    struct Impl;
    Impl* const impl;
};
