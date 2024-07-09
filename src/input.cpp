// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#include "input.hpp"

// --------------------------------------------------------------------------------------------------------------------

Input* createNewInput_LibInput();

// --------------------------------------------------------------------------------------------------------------------

Input* createNewInput(const InputBackendType type)
{
    switch (type)
    {
    case kInputBackendTypeNull:
        return nullptr;
    case kInputBackendTypeLibInput:
       #ifdef HAVE_LIBINPUT
        return createNewInput_LibInput();
       #else
        return nullptr;
       #endif
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
