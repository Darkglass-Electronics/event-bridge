// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#include "event-bridge.hpp"
#include "events.hpp"

EventInput* EventInput::createNew(const BackendType type, const char* const id, const uint8_t index)
{
    switch (type)
    {
    case kBackendTypeNull:
        return nullptr;
    case kBackendTypeGPIO:
        return createNewInput_GPIO(id, index);
    case kBackendTypeLibInput:
       #ifdef HAVE_LIBINPUT
        return createNewInput_LibInput(id);
       #else
        return nullptr;
       #endif
    case kBackendTypeLibSerialPort:
       #ifdef HAVE_LIBSERIALPORT
        return createNewInput_LibSerialPort(id);
       #else
        return nullptr;
       #endif
    }
    return nullptr;
}

EventOutput* EventOutput::createNew(const BackendType type, const char* const id)
{
    switch (type)
    {
    case kBackendTypeNull:
        return nullptr;
    case kBackendTypeGPIO:
        return createNewOutput_GPIO(id);
    case kBackendTypeSysfsLED:
        return createNewOutput_SysfsLED(id);
    }
    return nullptr;
}
