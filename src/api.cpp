// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "api.hpp"
#include "input.hpp"

#include <QtCore/QObject>

// --------------------------------------------------------------------------------------------------------------------

struct EventStream::Impl : QObject,
                           InputCallback
{
    Callbacks* const callbacks;
    std::vector<Input*> inputs;

    Impl(Callbacks* const callbacks, std::string& last_error)
        : callbacks(callbacks),
          last_error(last_error)
    {
        inputs.push_back(createNewInput(kInputTypeLibInput));
    }

    ~Impl()
    {
        for (Input* input : inputs)
            delete input;

        close();
    }

    void close()
    {
    }

    void poll()
    {
        for (Input* input : inputs)
            input->poll(this);
    }

private:
    std::string& last_error;

    void event(EventType etype, uint8_t index, int8_t value)
    {
        callbacks->inputEventReceived(etype, index, value);
    }
};

// --------------------------------------------------------------------------------------------------------------------

EventStream::EventStream(Callbacks* const callbacks)
    : impl(new Impl(callbacks, last_error)) {}

EventStream::~EventStream() { delete impl; }

void EventStream::poll()
{
    impl->poll();
}

// --------------------------------------------------------------------------------------------------------------------
