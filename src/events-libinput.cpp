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

struct LibInput : EventInput {
    struct libinput* context = nullptr;
    struct libinput_device* device = nullptr;
    int fd = -1;

    LibInput()
    {
        static constexpr const struct libinput_interface _interface = {
            .open_restricted = _open_restricted,
            .close_restricted = _close_restricted,
        };

        context = libinput_path_create_context(&_interface, nullptr);
        assert(context);

        fd = libinput_get_fd(context);
        assert(fd > 0);

        device = libinput_path_add_device(context, "/dev/input/by-path/platform-footswitches-event");

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
                case 44 ... 49:
                    cb->event(kEventTypeEncoder, keycode - 44, -1);
                    break;
                case 30 ... 35:
                    cb->event(kEventTypeEncoder, keycode - 30, 1);
                    break;
                case 2 ... 7:
                    cb->event(kEventTypeEncoder, keycode - 2, 0);
                    break;
                case 8 ... 10:
                    cb->event(kEventTypeFootswitch, keycode - 8,
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

EventInput* createNewInput_LibInput()
{
    return new LibInput();
}

// --------------------------------------------------------------------------------------------------------------------
