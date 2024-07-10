// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#include "event-bridge.hpp"
#include "events.hpp"

#include <cassert>
#include <cstdio>

// TODO test if file reopen is necessary

// --------------------------------------------------------------------------------------------------------------------

struct GPIOInput : EventInput {
    const uint8_t index;
    FILE* file = nullptr;
    int lastvalue = -1;

    GPIOInput(const uint16_t gpio, const uint8_t index_)
        : index(index_)
    {
        char path[32] = {};
        std::snprintf(path, sizeof(path) - 1, "/sys/class/gpio/gpio%u/value", gpio);
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
        std::fseek(file, 0, SEEK_SET);
        std::fscanf(file, "%d", &value);

        if (lastvalue != value)
        {
            lastvalue = value;
            cb->event(kEventTypeFootswitch, index, value);
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

struct GPIOOutput : EventOutput {
    FILE* file = nullptr;

    GPIOOutput(const uint16_t gpio)
    {
        char path[32] = {};
        std::snprintf(path, sizeof(path) - 1, "/sys/class/gpio/gpio%u/value", gpio);
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
        const int ivalue = value;
        std::fseek(file, 0, SEEK_SET);
        std::fwrite(&ivalue, sizeof(int), 1, file);
    }
};

// --------------------------------------------------------------------------------------------------------------------

EventInput* createNewInput_GPIO(const uint16_t gpio, const uint8_t index)
{
    return new GPIOInput(gpio, index);
}

EventOutput* createNewOutput_GPIO(const uint16_t gpio)
{
    return new GPIOOutput(gpio);
}

// --------------------------------------------------------------------------------------------------------------------
