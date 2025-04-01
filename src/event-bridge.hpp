// SPDX-FileCopyrightText: 2024-2025 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#pragma once

#include "events.hpp"

#include <cstdint>
#include <string>

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
        virtual void eventReceived(EventType etype, EventState state, uint8_t index, int32_t value) = 0;
    };

   /**
    * string describing the last error, in case any operation fails.
    */
    std::string last_error;

    /** constructor, optionally passing a callback for receiving events */
    EventBridge(Callback* callback = nullptr);

    /** destructor */
    virtual ~EventBridge();

    /**  */
    bool addInput(EventInput::BackendType type, const char* id, uint8_t index = 0);

    /**  */
    bool addOutput(EventOutput::BackendType type, const char* id, uint8_t index);

    /**
     * Clear current state, for preventing unwanted long-press events.
     */
    void clear();

    /**
     * Enable tap-tempo for a specific actuator.
     */
    void enableTapTempo(EventType etype, uint8_t index, bool enable = true);

    /**
     * Event polling function, to be called at regular intervals.
     * Will trigger event received callbacks if there were any events during the last period.
     * @note this function is very likely to be replaced with an FD-based event polling later on.
     */
    void poll();

    /**
     * Event trigger function, to be called for sending events.
     */
    bool sendEvent(EventType etype, uint8_t index, int32_t value);

private:
    struct Impl;
    Impl* const impl;
};
