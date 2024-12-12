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

struct LibSerialPort : EventInput {
    struct sp_port* serialport = nullptr;
    EventValue state[NUM_ENCODERS] = {};

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

    // FIXME read using thread?
    void poll(Callback* const cb) override
    {
        int ret;
        unsigned int offs = 0;
        char buf[0xff];

        ret = sp_blocking_read(serialport, buf, 2, SP_BLOCKING_READ_TIMEOUT);

        if (ret <= 0)
            return;

        // shift by 1 byte if message starts with a newline
        if (ret == 2 && buf[0] == '\n' && buf[1] != '\n')
        {
            buf[0] = buf[1];
            --ret;
        }

        // message was read in full, likely starting up and caught unflushed messages, we can stop here
        if (ret == 2 && buf[1] == '\n')
            return;

        for (offs = ret; offs < sizeof(buf); ++offs)
        {
            ret = sp_blocking_read(serialport, buf + offs, 1, SP_BLOCKING_READ_TIMEOUT);
            if (ret != 1)
                return;

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
                state[index] = buf[2] == '1' ? kEventValuePressed : kEventValueReleased;
            }

            cb->event(kEventTypeEncoder, state[index], index, value);
            break;
        }

        // TODO long press
    }
};

// --------------------------------------------------------------------------------------------------------------------

EventInput* createNewInput_LibSerialPort(const char* const path)
{
    return new LibSerialPort(path);
}

// --------------------------------------------------------------------------------------------------------------------
