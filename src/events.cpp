// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#include "event-bridge.hpp"
#include "events.hpp"

EventInput* EventInput::createNew(const BackendType type)
{
    switch (type)
    {
    case kBackendTypeNull:
        return nullptr;
    case kBackendTypeGPIO:
        return createNewInput_GPIO(0, 0);
    case kBackendTypeLibInput:
       #ifdef HAVE_LIBINPUT
        return createNewInput_LibInput();
       #else
        return nullptr;
       #endif
    }
    return nullptr;
}

EventOutput* EventOutput::createNew(const BackendType type)
{
    switch (type)
    {
    case kBackendTypeNull:
        return nullptr;
    case kBackendTypeGPIO:
        return createNewOutput_GPIO(0);
    }
    return nullptr;
}
