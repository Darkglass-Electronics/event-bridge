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
 * Default tap-tempo hysteresis in milliseconds.
 * If time between taps takes longer than this hysteresis, absolute delta-time is used as tap-tempo.
 * Otherwise a running average is used.
 */
#ifndef EVENT_BRIDGE_TAP_TEMPO_HYSTERESIS
#define EVENT_BRIDGE_TAP_TEMPO_HYSTERESIS 750
#endif

/**
 * Default tap-tempo timeout in milliseconds.
 * If time between taps takes longer than this timeout, they are ignored.
 */
#ifndef EVENT_BRIDGE_TAP_TEMPO_TIMEOUT
#define EVENT_BRIDGE_TAP_TEMPO_TIMEOUT 3000
#endif

/**
 * Default tap-tempo timeout overflow in milliseconds.
 * Taps will be accepted and clamped to EVENT_BRIDGE_TAP_TEMPO_TIMEOUT if overflowing by this amount.
 */
#ifndef EVENT_BRIDGE_TAP_TEMPO_TIMEOUT_OVERFLOW
#define EVENT_BRIDGE_TAP_TEMPO_TIMEOUT_OVERFLOW 50
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
 * @see EventState
 */
enum EventType {
    /** Null event type. */
    kEventTypeNull = 0,

    /**
     * Encoder rotation event type, an endless rotation actuator not bound to a minimum/maximum range.
     * Positive values mean clock-wise rotation, negative values mean anti-clock-wise rotation.
     */
    kEventTypeEncoder,

    /** Footswitch event type. */
    kEventTypeFootswitch,

    /** TBD. */
    kEventTypeLED,
};

/**
 * The possible event states.
 * @see EventType
 */
enum EventState {
    /** Actuator is "up" or released. */
    kEventStateReleased = 0,

    /** Actuator is "down" or pressed. */
    kEventStatePressed,

    /** Footswitch has been pressed for a "long" time, defined by @a EVENT_BRIDGE_LONG_PRESS_TIME. */
    kEventStateLongPressed,

    /** Actuator updated tap-tempo value. */
    kEventStateTapTempo,
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
const char* EventStateStr(const EventState state)
{
    switch (state)
    {
    case kEventStateReleased:
        return "kEventStateReleased";
    case kEventStatePressed:
        return "kEventStatePressed";
    case kEventStateLongPressed:
        return "kEventStateLongPressed";
    case kEventStateTapTempo:
        return "kEventStateTapTempo";
    }
    return "";
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
        virtual void event(EventType etype, EventState evalue, uint8_t index, int32_t value) = 0;
    };

    /** destructor */
    virtual ~EventInput() {};

    /**
     * Clear current state, for preventing unwanted long-press events.
     */
    virtual void clear() {}

    /**
     * Enable tap-tempo for a specific actuator.
     */
    virtual void enableTapTempo(uint8_t index, bool enable) = 0;

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
    virtual void event(int32_t value) = 0;

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
