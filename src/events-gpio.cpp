// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#include "event-bridge.hpp"
#include "events.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>

// --------------------------------------------------------------------------------------------------------------------

struct GPIOInput : EventInput {
    const uint8_t _index;
    FILE* file = nullptr;
    int lastvalue = -1;

    GPIOInput(const char* const id, const uint8_t index)
        : _index(index)
    {
        char path[48] = {};
        std::snprintf(path, sizeof(path) - 1, "/sys/class/gpio/gpio%s/value", id);
        file = std::fopen(path, "r");
        assert(file != nullptr);
    }

    ~GPIOInput() override
    {
        if (file != nullptr)
            std::fclose(file);
    }

    // FIXME timer poll is bad, rework API to work via FDs directly
    void poll(Callback* const cb) override
    {
        int value = 0;

        if (file != nullptr)
        {
            std::fseek(file, 0, SEEK_SET);
            std::fscanf(file, "%d", &value);
        }

        if (lastvalue != value)
        {
            lastvalue = value;
            cb->event(kEventTypeFootswitch, value != 0 ? kEventStatePressed : kEventStateReleased, _index, 0);
        }

        // TODO long press
    }
};

// --------------------------------------------------------------------------------------------------------------------

struct GPIOOutput : EventOutput {
    FILE* file = nullptr;

    GPIOOutput(const char* const id)
    {
        char path[48] = {};
        std::snprintf(path, sizeof(path) - 1, "/sys/class/gpio/gpio%s/value", id);
        file = std::fopen(path, "w");
        assert(file != nullptr);
    }

    ~GPIOOutput() override
    {
        if (file != nullptr)
            std::fclose(file);
    }

    void event(const int16_t value) override
    {
        if (file != nullptr)
        {
            char svalue[12] = {};
            std::snprintf(svalue, sizeof(svalue) - 1, "%d", value);

            std::fseek(file, 0, SEEK_SET);
            std::fwrite(svalue, std::strlen(svalue) + 1, 1, file);
            std::fflush(file);
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

EventInput* createNewInput_GPIO(const char* const id, const uint8_t index)
{
    return new GPIOInput(id, index);
}

EventOutput* createNewOutput_GPIO(const char* const id)
{
    return new GPIOOutput(id);
}

// --------------------------------------------------------------------------------------------------------------------
