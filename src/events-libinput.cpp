// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#include "event-bridge.hpp"
#include "events.hpp"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <ctime>
#include <fcntl.h>
#include <libinput.h>
#include <poll.h>
#include <unistd.h>

// --------------------------------------------------------------------------------------------------------------------

#ifndef ENCODER_CLICK_START
#define ENCODER_CLICK_START 16
#endif

#ifndef ENCODER_LEFT_START
#define ENCODER_LEFT_START 30
#endif

#ifndef ENCODER_RIGHT_START
#define ENCODER_RIGHT_START 44
#endif

#ifndef FOOTSWITCH_CLICK_START
#define FOOTSWITCH_CLICK_START 101
#endif

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

struct LibInput : EventInput {
    struct libinput* context = nullptr;
    struct libinput_device* device = nullptr;
    int fd = -1;
    struct {
        uint32_t time;
        EventState value;
    } state[NUM_ENCODERS + NUM_FOOTSWITCHES] = {};

    LibInput(const char* const path)
    {
        static constexpr const struct libinput_interface _interface = {
            .open_restricted = _open_restricted,
            .close_restricted = _close_restricted,
        };

        context = libinput_path_create_context(&_interface, nullptr);
        assert(context);

        fd = libinput_get_fd(context);
        assert(fd > 0);

        device = libinput_path_add_device(context, path);

        if (device == nullptr)
            return;

        // device = 
        libinput_device_ref(device);
    }

    ~LibInput() override
    {
        if (device != nullptr)
        {
            libinput_path_remove_device(device);
            libinput_device_unref(device);
        }

        if (context != nullptr)
            libinput_unref(context);
    }

    void clear() override
    {
        for (int i = 0; i < sizeof(state)/sizeof(state[0]); ++i)
        {
            state[i].time = 0;
            state[i].value = kEventStateReleased;
        }
    }

    // FIXME timer poll is bad, rework API to work via FDs directly
    void poll(Callback* const cb) override
    {
        static constexpr const int timeout = 0;

        struct pollfd fds[1] = {};
        fds[0].fd = fd;
        fds[0].events = POLLIN;
        fds[0].revents = 0;

        const int rc = ::poll(fds, 1, timeout);
        if (rc == 0)
        {
            updateLongPresses(cb);
            return;
        }

        libinput_dispatch(context);

        for (struct libinput_event* event; (event = libinput_get_event(context)) != nullptr;)
        {
            // printf("event %p\n", event);

            if (libinput_event_get_type(event) == LIBINPUT_EVENT_KEYBOARD_KEY)
            {
                struct libinput_event_keyboard* const keyevent = libinput_event_get_keyboard_event(event);
                const uint32_t keycode = libinput_event_keyboard_get_key(keyevent);
                uint8_t index;

                switch (keycode)
                {
                case ENCODER_CLICK_START ... ENCODER_CLICK_START + NUM_ENCODERS:
                    index = keycode - ENCODER_CLICK_START;
                    if (libinput_event_keyboard_get_key_state(keyevent) == LIBINPUT_KEY_STATE_PRESSED)
                    {
                        state[index].time = get_time_ms();
                        state[index].value = kEventStatePressed;
                    }
                    else
                    {
                        state[index].time = 0;
                        state[index].value = kEventStateReleased;
                    }
                    cb->event(kEventTypeEncoder, state[index].value, index, 0);
                    break;

                case ENCODER_LEFT_START ... ENCODER_LEFT_START + NUM_ENCODERS:
                    index = keycode - ENCODER_LEFT_START;
                    cb->event(kEventTypeEncoder, state[index].value, index, -1);
                    break;

                case ENCODER_RIGHT_START ... ENCODER_RIGHT_START + NUM_ENCODERS:
                    index = keycode - ENCODER_RIGHT_START;
                    cb->event(kEventTypeEncoder, state[index].value, index, 1);
                    break;

                case FOOTSWITCH_CLICK_START ... FOOTSWITCH_CLICK_START + NUM_FOOTSWITCHES:
                    index = keycode - FOOTSWITCH_CLICK_START;
                    if (libinput_event_keyboard_get_key_state(keyevent) == LIBINPUT_KEY_STATE_PRESSED)
                    {
                        state[NUM_ENCODERS + index].time = get_time_ms();
                        state[NUM_ENCODERS + index].value = kEventStatePressed;
                    }
                    else
                    {
                        state[NUM_ENCODERS + index].time = 0;
                        state[NUM_ENCODERS + index].value = kEventStateReleased;
                    }
                    cb->event(kEventTypeFootswitch, state[NUM_ENCODERS + index].value, index, 0);
                    break;

                default:
                    printf("unused event keycode %d\n", keycode);
                    break;
                }
            }

            libinput_event_destroy(event);
        }

        updateLongPresses(cb);
    }

private:
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

    static int _open_restricted(const char* const path, const int flags, void*)
    {
        const int fd = open(path, flags);
        return fd < 0 ? -errno : fd;
    }

    static void _close_restricted(int fd, void*)
    {
        close(fd);
    }
};

// --------------------------------------------------------------------------------------------------------------------

EventInput* createNewInput_LibInput(const char* const path)
{
    return new LibInput(path);
}

// --------------------------------------------------------------------------------------------------------------------
