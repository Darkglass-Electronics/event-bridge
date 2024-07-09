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
 * Abstract Event class for sending and receiving events.
 * TODO better naming
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
     * Create a new Input class for a specified event-handling backend.
     */
    static EventInput* createNew(BackendType type);
};

struct EventOutput {
    /**
     * The possible backends for handling events.
     */
    enum BackendType {
        kBackendTypeNull,
        kBackendTypeGPIO,
    };

    /** destructor */
    virtual ~EventOutput() {};

    /**
     * Event trigger function, to be called for sending events.
     */
    virtual void event(EventType type, uint8_t index, int16_t value) = 0;

    /**
     * Create a new Input class for a specified event-handling backend.
     */
    static EventOutput* createNew(BackendType type);
};

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
    void sendEvent(EventType etype, uint8_t index, int16_t value);

private:
    struct Impl;
    Impl* const impl;
};
