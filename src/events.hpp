// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#pragma once

#include <cstdint>

/**
 * Default time in milliseconds for a long-press event.
 */
#ifndef EVENT_BRIDGE_LONG_PRESS_TIME
#define EVENT_BRIDGE_LONG_PRESS_TIME 500
#endif

/**
 * Default number of encoders to use.
 */
#ifndef NUM_ENCODERS
#define NUM_ENCODERS 6
#endif

/**
 * Default number of footswitches to use.
 */
#ifndef NUM_FOOTSWITCHES
#define NUM_FOOTSWITCHES 3
#endif

/**
 * Default number of LEDs to use.
 */
#ifndef NUM_LEDS
#define NUM_LEDS 3
#endif

/**
 * The possible event types, for both receiving and sending.
 * @see EventValue
 */
enum EventType {
    /** Null event type. */
    kEventTypeNull = 0,

    /**
     * Encoder rotation event type, an endless rotation actuator not bound to a minimum/maximum range.
     * Positive values mean clock-wise rotation, negative values mean anti-clock-wise rotation.
     */
    kEventTypeEncoder,

    /**
     * Footswitch event type.
     * @see EventValue
     */
    kEventTypeFootswitch,

    /**
     * TBD.
     */
    kEventTypeLED,
};

/**
 * The possible event values.
 * @see EventType
 */
enum EventValue {
    /** Footswitch is "up" or released. */
    kEventValueReleased = 0,

    /** Footswitch is "down" or pressed. */
    kEventValuePressed,

    /** Footswitch has been pressed for a "long" time, defined by @a EVENT_BRIDGE_LONG_PRESS_TIME. */
    kEventValueLongPressed,
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
 * Convenience function to convert an event value to a string.
 */
static constexpr inline
const char* EventValueStr(const EventValue evalue)
{
    switch (evalue)
    {
    case kEventValueReleased:
        return "kEventValueReleased";
    case kEventValuePressed:
        return "kEventValuePressed";
    case kEventValueLongPressed:
        return "kEventValueLongPressed";
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
 * Convenience function to convert a 32-bit color value into 16-bit.
 */
static constexpr inline
int16_t led_rgb_value(const uint32_t rgb)
{
    return (rgb & 0xf00000) >> 12 | (rgb & 0xf000) >> 8 | (rgb & 0xf0) >> 4;
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
        kBackendTypeLibSerialPort,
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
        virtual void event(EventType etype, EventValue evalue, uint8_t index, int16_t value) = 0;
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
#ifdef HAVE_LIBSERIALPORT
EventInput* createNewInput_LibSerialPort(const char* path);
#endif

EventOutput* createNewOutput_GPIO(const char* id);
EventOutput* createNewOutput_SysfsLED(const char* id);
