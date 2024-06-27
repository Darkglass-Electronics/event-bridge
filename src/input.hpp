// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#pragma once

#include <cstdint>

/**
 * TODO document me
 */
enum EventType {
    kEventTypeNull = 0,
    kEventTypeEncoder,
    kEventTypeFootswitch,
    kEventTypeKnob,
    kEventTypeLED,
};

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
    case kEventTypeKnob:
        return "kEventTypeKnob";
    case kEventTypeLED:
        return "kEventTypeLED";
    }
    return "";
}

/**
 * TODO document me
 */
enum InputType {
    kInputTypeNull,
    kInputTypeLibInput,
};

/**
 * TODO document me
 */
struct InputCallback {
    virtual ~InputCallback() {};

    /**
     * Event trigger function, called when an event is received.
     */
    virtual void event(EventType etype, uint8_t index, int8_t value) = 0;
};

/**
 * TODO document me
 */
struct Input {
    virtual ~Input() {};

    /**
     * Event polling function, to be called at regular intervals.
     * @note function very likely to be replaced with FD-based event polling later on.
     */
    virtual void poll(InputCallback* cb) = 0;

    /**
     * Event trigger function, to be called for sending events.
     */
    virtual void event(EventType etype, uint8_t index, int8_t value) = 0;
};

/**
 * TODO document me
 */
Input* createNewInput(InputType itype);
