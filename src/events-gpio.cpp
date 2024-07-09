// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#include "bridge.hpp"
#include "events.hpp"

#include <cassert>
#include <cstdio>

// TODO test if file reopen is necessary

// --------------------------------------------------------------------------------------------------------------------

struct GPIOInput : EventInput {
    const int index;
    FILE* file = nullptr;

    GPIOInput(const int gpio, const int index_)
        : index(index_)
    {
        char path[32] = {};
        std::snprintf(path, sizeof(path) - 1, "/sys/class/gpio/gpio%d/value", gpio);
        file = fopen(path, "r");
        assert(file != nullptr);
    }

    ~GPIOInput() override
    {
        if (file != nullptr)
            fclose(file);
    }

    // FIXME timer poll is bad, rework API to work via FDs directly
    void poll(Callback* const cb) override
    {
        int value = 0;
        fseek(file, 0, SEEK_SET);
        fscanf(file, "%d", &value);

        cb->event(kEventTypeFootswitch, index, value);
    }
};

// --------------------------------------------------------------------------------------------------------------------

struct GPIOOutput : EventOutput {
    FILE* file = nullptr;

    GPIOOutput(const int gpio)
    {
        char path[32] = {};
        std::snprintf(path, sizeof(path) - 1, "/sys/class/gpio/gpio%d/value", gpio);
        file = fopen(path, "w");
        assert(file != nullptr);
    }

    ~GPIOOutput() override
    {
        if (file != nullptr)
            fclose(file);
    }

    void event(EventType, uint8_t, const int16_t value) override
    {
        const int ivalue = value;
        fseek(file, 0, SEEK_SET);
        fwrite(&ivalue, sizeof(int), 1, file);
    }
};

// --------------------------------------------------------------------------------------------------------------------

EventInput* createNewInput_GPIO(const int gpio, const int index)
{
    return new GPIOInput(gpio, index);
}

EventOutput* createNewOutput_GPIO(const int gpio)
{
    return new GPIOOutput(gpio);
}

// --------------------------------------------------------------------------------------------------------------------
