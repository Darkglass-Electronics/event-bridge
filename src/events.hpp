// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#pragma once

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
    static EventInput* createNew(BackendType type);
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
    };

    /** destructor */
    virtual ~EventOutput() {};

    /**
     * Event trigger function, to be called for sending events.
     */
    virtual void event(EventType type, uint8_t index, int16_t value) = 0;

    /**
     * Entry point.
     * Creates a new EventOutput class for a specified event-handling backend.
     */
    static EventOutput* createNew(BackendType type);
};

EventInput* createNewInput_GPIO(int gpio, int index);
EventOutput* createNewOutput_GPIO(int gpio);
#ifdef HAVE_LIBINPUT
EventInput* createNewInput_LibInput();
#endif
