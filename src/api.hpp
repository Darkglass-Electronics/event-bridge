// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>

/**
 * TODO document me
 */
struct EventStream {
   /**
    * string describing the last error, in case any operation fails.
    */
    std::string last_error;

    EventStream();
    ~EventStream();

private:
    struct Impl;
    Impl* const impl;
};
