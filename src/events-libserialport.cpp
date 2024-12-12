// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#include "event-bridge.hpp"
#include "events.hpp"

#include <cassert>
#include <cstdio>

#include <libserialport.h>

// --------------------------------------------------------------------------------------------------------------------

// in ms
#define SP_BLOCKING_READ_TIMEOUT 1

// --------------------------------------------------------------------------------------------------------------------

static uint32_t get_time_ms() noexcept
{
    static struct {
        timespec ts;
        int r;
        uint32_t ms;
    } s = {
        {},
        clock_gettime(CLOCK_MONOTONIC_RAW, &s.ts),
        static_cast<uint32_t>(s.ts.tv_sec * 1000 + s.ts.tv_nsec / 1000000)
    };

    timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000) - s.ms;
}

// --------------------------------------------------------------------------------------------------------------------

struct LibSerialPort : EventInput {
    struct sp_port* serialport = nullptr;
    struct {
        uint32_t time;
        EventState value;
    } state[NUM_ENCODERS] = {};

    LibSerialPort(const char* const path, const int baudrate = 115200)
    {
        enum sp_return ret;

        // get serial port
        ret = sp_get_port_by_name(path, &serialport);
        if (ret != SP_OK)
        {
            fprintf(stderr, "%s failed, cannot get serial port for device '%s'\n", __func__, path);
            return;
        }

        // open serial port
        ret = sp_open(serialport, SP_MODE_READ_WRITE);
        if (ret != SP_OK)
        {
            fprintf(stderr, "%s failed, cannot open serial port for device '%s'\n", __func__, path);
            sp_free_port(serialport);
            serialport = nullptr;
            return;
        }

        // disable XON/XOFF flow control
        sp_set_xon_xoff(serialport, SP_XONXOFF_DISABLED);

        // configure serial port
        sp_set_baudrate(serialport, baudrate);
    }

    ~LibSerialPort() override
    {
        if (serialport == nullptr)
            return;

        sp_close(serialport);
        sp_free_port(serialport);
    }

    void clear() override
    {
        for (int i = 0; i < sizeof(state)/sizeof(state[0]); ++i)
        {
            state[i].time = 0;
            state[i].value = kEventStateReleased;
        }
    }

    // FIXME read using thread?
    void poll(Callback* const cb) override
    {
        int ret;
        unsigned int offs = 0;
        char buf[0xff];

        ret = sp_blocking_read(serialport, buf, 2, SP_BLOCKING_READ_TIMEOUT);

        if (ret <= 0)
        {
            updateLongPresses(cb);
            return;
        }

        // shift by 1 byte if message starts with a newline
        if (ret == 2 && buf[0] == '\n' && buf[1] != '\n')
        {
            buf[0] = buf[1];
            --ret;
        }

        // message was read in full, likely starting up and caught unflushed messages, we can stop here
        if (ret == 2 && buf[1] == '\n')
        {
            updateLongPresses(cb);
            return;
        }

        for (offs = ret; offs < sizeof(buf); ++offs)
        {
            ret = sp_blocking_read(serialport, buf + offs, 1, SP_BLOCKING_READ_TIMEOUT);
            if (ret != 1)
            {
                updateLongPresses(cb);
                return;
            }

            // still reading
            if (buf[offs] != '\n')
                continue;

            // got full message
            const char c = buf[0];

            // debugging tests
            assert(offs >= 3);
            assert((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
            assert(buf[1] == ' ');

            uint8_t index;
            int16_t value;

            if (c >= 'A' && c <= 'Z')
            {
                assert(buf[2] == '-' || buf[2] == '+');

                buf[offs] = '\0';
                index = c - 'A';
                assert(index < NUM_ENCODERS);
                value = std::atoi(buf + 2);
            }
            else // if (c >= 'a' && c <= 'z')
            {
                assert(buf[2] == '0' || buf[2] == '1');

                index = c - 'a';
                assert(index < NUM_ENCODERS);
                value = 0;
                if (buf[2] == '1')
                {
                    state[index].time = get_time_ms();
                    state[index].value = kEventStatePressed;
                }
                else
                {
                    state[index].time = 0;
                    state[index].value = kEventStateReleased;
                }
            }

            cb->event(kEventTypeEncoder, state[index].value, index, value);
            break;
        }

        updateLongPresses(cb);
    }

    void updateLongPresses(Callback* const cb)
    {
        // only ask for current time as needed
        uint32_t now = 0;

        for (int i = 0; i < sizeof(state)/sizeof(state[0]); ++i)
        {
            if (state[i].value != kEventStatePressed)
                continue;

            if (now == 0)
                now = get_time_ms();

            if (now - state[i].time < EVENT_BRIDGE_LONG_PRESS_TIME)
                continue;

            state[i].time = 0;
            state[i].value = kEventStateLongPressed;

            if (i < NUM_ENCODERS)
                cb->event(kEventTypeEncoder, state[i].value, i, 0);
            else
                cb->event(kEventTypeFootswitch, state[i].value, i - NUM_ENCODERS, 0);
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

EventInput* createNewInput_LibSerialPort(const char* const path)
{
    return new LibSerialPort(path);
}

// --------------------------------------------------------------------------------------------------------------------
