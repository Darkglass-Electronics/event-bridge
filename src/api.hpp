// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include "input.hpp"

#include <string>

/**
 * TODO document me
 */
struct EventStream {
    struct Callbacks {
        virtual ~Callbacks() {}
        virtual void inputEventReceived(EventType etype, uint8_t index, int8_t value) = 0;
    };

   /**
    * string describing the last error, in case any operation fails.
    */
    std::string last_error;

    EventStream(Callbacks* callbacks);
    ~EventStream();

    void poll();

private:
    struct Impl;
    Impl* const impl;
};
