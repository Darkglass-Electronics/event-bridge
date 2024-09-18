// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#pragma once

#include <cstdint>

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
 * Convenience function to convert separate r, g, b values into a single 16-bit color value.
 * Color values must be between 0 and 0xf
 */
static constexpr inline
int16_t led_rgb_value(const int8_t r, const int8_t g, const int8_t b)
{
    return (r & 0xf) << 8 | (g & 0xf) << 4 | (b & 0xf);
}

/**
 * Abstract Event class for receiving events.
 */
struct EventInput {
    /**
     * The possible backends for handling events.
     */
    enum BackendType {
        kBackendTypeNull,
        kBackendTypeGPIO,
        kBackendTypeLibInput,
    };

    /**
     * Callback used for receiving events, triggered via poll().
     */
    struct Callback {
        /** destructor */
        virtual ~Callback() {};

        /**
         * Event trigger function, called when an event is received.
         */
        virtual void event(EventType type, uint8_t index, int16_t value) = 0;
    };

    /** destructor */
    virtual ~EventInput() {};

    /**
     * Event polling function, to be called at regular intervals.
     * @note this function is very likely to be replaced with an FD-based event polling later on.
     */
    virtual void poll(Callback* cb) = 0;

    /**
     * Entry point.
     * Creates a new EventInput class for a specified event-handling backend.
     */
    static EventInput* createNew(BackendType type, const char* id, uint8_t index);
};

/**
 * Abstract Event class for sending events.
 */
struct EventOutput {
    /**
     * The possible backends for handling events.
     */
    enum BackendType {
        kBackendTypeNull,
        kBackendTypeGPIO,
        kBackendTypeSysfsLED,
    };

    /** destructor */
    virtual ~EventOutput() {};

    /**
     * Event trigger function, to be called for sending events.
     */
    virtual void event(int16_t value) = 0;

    /**
     * Entry point.
     * Creates a new EventOutput class for a specified event-handling backend.
     */
    static EventOutput* createNew(BackendType type, const char* id);
};

EventInput* createNewInput_GPIO(const char* id, uint8_t index);
#ifdef HAVE_LIBINPUT
EventInput* createNewInput_LibInput(const char* id);
#endif

EventOutput* createNewOutput_GPIO(const char* id);
EventOutput* createNewOutput_SysfsLED(const char* id);
