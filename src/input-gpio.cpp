// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#include "input.hpp"

#include <cassert>
#include <cstdio>

// TODO test if file reopen is necessary

// --------------------------------------------------------------------------------------------------------------------

struct GPIOInput : Input {
    FILE* file = nullptr;

    GPIOInput()
    {
        file = fopen("/sys/class/gpio/gpio1/value", "r");
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

        cb->event(kEventTypeFootswitch, 0, value);
    }

    void event(EventType, uint8_t, int16_t) override
    {
        // this class is read-only, nothing to do here
    }
};

// --------------------------------------------------------------------------------------------------------------------

struct GPIOOutput : Input {
    FILE* file = nullptr;

    GPIOOutput()
    {
        file = fopen("/sys/class/gpio/gpio1/value", "w");
        assert(file != nullptr);
    }

    ~GPIOOutput() override
    {
        if (file != nullptr)
            fclose(file);
    }

    // FIXME timer poll is bad, rework API to work via FDs directly
    void poll(Callback* const cb) override
    {
        // this class is write-only, nothing to do here
    }

    void event(EventType, uint8_t, int16_t value) override
    {
        const int ivalue = value;
        fseek(file, 0, SEEK_SET);
        fwrite(&ivalue, sizeof(int), 1, file);
    }
};

// --------------------------------------------------------------------------------------------------------------------

Input* createNewInput_GPIOInput()
{
    return new GPIOInput();
}

Input* createNewInput_GPIOOutput()
{
    return new GPIOOutput();
}

// --------------------------------------------------------------------------------------------------------------------
