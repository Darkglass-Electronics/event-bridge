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
        return createNewInput_LibInput();
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
