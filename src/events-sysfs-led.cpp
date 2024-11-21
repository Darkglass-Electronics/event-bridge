// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#include "event-bridge.hpp"
#include "events.hpp"

#include <cstdio>
#include <cstring>

// --------------------------------------------------------------------------------------------------------------------

enum LED {
    kRed = 0,
    kGreen,
    kBlue,
    kNumLEDs
};

static constexpr const char* kColors[kNumLEDs] = {
    "red", "green", "blue",
};

struct SysfsLED : EventOutput {
    FILE* files[kNumLEDs] = {};
    int lastValues[kNumLEDs] = {};
    int maxBrightness[kNumLEDs] = {};

    SysfsLED(const char* const id)
    {
        for (int i = 0; i < kNumLEDs; ++i)
            initLED(id, i);
    }

    ~SysfsLED() override
    {
        for (int i = 0; i < kNumLEDs; ++i)
        {
            if (files[i] != nullptr)
                std::fclose(files[i]);
        }
    }

    void event(const int16_t value) override
    {
        const uint8_t r = (value >> 8) & 0xf;
        const uint8_t g = (value >> 4) & 0xf;
        const uint8_t b =  value & 0xf;
        const int values[kNumLEDs] = { r, g, b };
        char svalue[12] = {};

        for (int i = 0; i < kNumLEDs; ++i)
        {
            if (lastValues[i] == values[i])
                continue;

            lastValues[i] = values[i];

            if (files[i] != nullptr)
            {
                const int ivalue = static_cast<float>(values[i]) / 0xf * maxBrightness[i] * 0.1f;
                std::snprintf(svalue, sizeof(svalue) - 1, "%d", ivalue);

                std::fseek(files[i], 0, SEEK_SET);
                std::fwrite(svalue, std::strlen(svalue) + 1, 1, files[i]);
                std::fflush(files[i]);
            }
        }
    }

private:
    void initLED(const char* const id, const int ledn)
    {
        const char* const color = kColors[ledn];
        char path[48] = {};

        std::snprintf(path, sizeof(path) - 1, "/sys/class/leds/%s:%s/max_brightness", id, color);
        if (FILE* const file = std::fopen(path, "r"))
        {
            std::fseek(file, 0, SEEK_SET);
            std::fscanf(file, "%d", &maxBrightness[ledn]);
            std::fclose(file);

            std::snprintf(path, sizeof(path) - 1, "/sys/class/leds/%s:%s/brightness", id, color);
            files[ledn] = std::fopen(path, "w");

            // start with minimum brightness
            if (files[ledn] != nullptr)
            {
                const char svalue[] = { '0', '\0' };
                std::fseek(files[ledn], 0, SEEK_SET);
                std::fwrite(svalue, sizeof(svalue), 1, files[ledn]);
                std::fflush(files[ledn]);
            }
            else
            {
                fprintf(stderr, "cannot open LED %s for color %s\n", id, color);
            }
        }
        else
        {
            fprintf(stderr, "cannot open LED max_brightness %s for color %s\n", id, color);
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

EventOutput* createNewOutput_SysfsLED(const char* const id)
{
    return new SysfsLED(id);
}

// --------------------------------------------------------------------------------------------------------------------
