// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#include "event-bridge.hpp"
#include "events.hpp"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <libinput.h>
#include <poll.h>
#include <unistd.h>

// --------------------------------------------------------------------------------------------------------------------

#ifndef NUM_ENCODERS
#define NUM_ENCODERS 6
#endif

#ifndef NUM_FOOTSWITCHES
#define NUM_FOOTSWITCHES 3
#endif

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

struct LibInput : EventInput {
    struct libinput* context = nullptr;
    struct libinput_device* device = nullptr;
    int fd = -1;

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
            return;

        libinput_dispatch(context);

        for (struct libinput_event* event; (event = libinput_get_event(context)) != nullptr;)
        {
            // printf("event %p\n", event);

            if (libinput_event_get_type(event) == LIBINPUT_EVENT_KEYBOARD_KEY)
            {
                struct libinput_event_keyboard* const keyevent = libinput_event_get_keyboard_event(event);
                const uint32_t keycode = libinput_event_keyboard_get_key(keyevent);

                switch (keycode)
                {
                case ENCODER_CLICK_START ... ENCODER_CLICK_START + NUM_ENCODERS:
                    if (libinput_event_keyboard_get_key_state(keyevent) == LIBINPUT_KEY_STATE_PRESSED)
                        cb->event(kEventTypeEncoder, keycode - ENCODER_CLICK_START, 0);
                    break;

                case ENCODER_LEFT_START ... ENCODER_LEFT_START + NUM_ENCODERS:
                    cb->event(kEventTypeEncoder, keycode - ENCODER_LEFT_START, -1);
                    break;

                case ENCODER_RIGHT_START ... ENCODER_RIGHT_START + NUM_ENCODERS:
                    cb->event(kEventTypeEncoder, keycode - ENCODER_RIGHT_START, 1);
                    break;

                case FOOTSWITCH_CLICK_START ... FOOTSWITCH_CLICK_START + NUM_FOOTSWITCHES:
                    cb->event(kEventTypeFootswitch, keycode - FOOTSWITCH_CLICK_START,
                              libinput_event_keyboard_get_key_state(keyevent) == LIBINPUT_KEY_STATE_PRESSED ? 1 : 0);
                    break;

                default:
                    printf("unused event keycode %d\n", keycode);
                    break;
                }
            }

            libinput_event_destroy(event);
        }
    }

private:
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
