// SPDX-FileCopyrightText: 2024-2025 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#include "event-bridge.hpp"
#include "events.hpp"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <ctime>
#include <vector>

#include <fcntl.h>
#include <libinput.h>
#include <poll.h>
#include <pthread.h>
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

static uint32_t abs_delta(const uint32_t a, const uint32_t b) noexcept
{
    return a > b ? b - a : a - b;
}

// --------------------------------------------------------------------------------------------------------------------

struct LibInput : EventInput {
    struct libinput* context = nullptr;
    struct libinput_device* device = nullptr;
    int fd = -1;
    struct State {
        uint32_t time = 0;
        EventState value = kEventStateReleased;
    } state[NUM_ENCODERS + NUM_FOOTSWITCHES];
    struct TapTempo {
        uint64_t time = 0;
        uint32_t value = 0;
        bool enabled = false;
        bool updated = false;
    } tapTempo[NUM_ENCODERS + NUM_FOOTSWITCHES];

    struct QueueEvent {
        EventType etype;
        EventState evalue;
        uint8_t index;
        int32_t value;
    };
    std::vector<QueueEvent> events;

    // copies
    std::vector<QueueEvent> events2;
    TapTempo tapTempo2[NUM_ENCODERS + NUM_FOOTSWITCHES];

    pthread_mutex_t lock = {};

    struct {
        pthread_t handle = {};
        bool running = false;
    } thread;

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

        libinput_device_ref(device);

        pthread_mutex_init(&lock, nullptr);

        thread.running = true;
        if (pthread_create(&thread.handle, nullptr, _run, this) != 0)
            thread.running = false;
    }

    ~LibInput() override
    {
        if (thread.running)
        {
            thread.running = false;
            pthread_join(thread.handle, nullptr);
        }

        pthread_mutex_destroy(&lock);

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
        pthread_mutex_lock(&lock);

        for (int i = 0; i < sizeof(state)/sizeof(state[0]); ++i)
        {
            state[i].time = 0;
            state[i].value = kEventStateReleased;
        }

        for (int i = 0; i < sizeof(tapTempo)/sizeof(tapTempo[0]); ++i)
        {
            tapTempo[i].time = 0;
            tapTempo[i].value = 0;
            tapTempo[i].enabled = false;
            tapTempo[i].updated = false;
        }

        events.clear();

        pthread_mutex_unlock(&lock);
    }

    void enableTapTempo(const uint8_t index, const bool enable) override
    {
        assert(index < NUM_ENCODERS + NUM_FOOTSWITCHES);

        pthread_mutex_lock(&lock);

        tapTempo[index].time = 0;
        tapTempo[index].value = 0;
        tapTempo[index].enabled = enable;
        tapTempo[index].updated = false;

        pthread_mutex_unlock(&lock);
    }

    void poll(Callback* const cb) override
    {
        if (! thread.running)
            readInput(1);

        copy2();

        for (const QueueEvent& ev : events2)
            cb->event(ev.etype, ev.evalue, ev.index, ev.value);

        for (int i = 0; i < sizeof(tapTempo2)/sizeof(tapTempo2[0]); ++i)
        {
            if (! tapTempo2[i].updated)
                continue;

            tapTempo2[i].updated = false;

            if (i < NUM_ENCODERS)
                cb->event(kEventTypeEncoder, kEventStateTapTempo, i, tapTempo2[i].value);
            else
                cb->event(kEventTypeFootswitch, kEventStateTapTempo, i - NUM_ENCODERS, tapTempo2[i].value);
        }
    }

private:
    void copy2()
    {
        pthread_mutex_lock(&lock);

        events.swap(events2);
        events.clear();

        for (int i = 0; i < sizeof(tapTempo)/sizeof(tapTempo[0]); ++i)
        {
            if (tapTempo[i].updated)
            {
                tapTempo2[i] = tapTempo[i];
                tapTempo[i].updated = false;
            }
        }

        pthread_mutex_unlock(&lock);
    }

    static void* _run(void* const arg)
    {
        static_cast<LibInput*>(arg)->run();
        return nullptr;
    }

    void run()
    {
        static constexpr const uint32_t timeoutMs = 100;

        while (thread.running)
            readInput(timeoutMs);
    }

    void readInput(const uint32_t timeoutMs)
    {
        struct pollfd fds[1] = {};
        fds[0].fd = fd;
        fds[0].events = POLLIN;
        fds[0].revents = 0;

        const int rc = ::poll(fds, 1, timeoutMs);

        if (rc == 0)
        {
            updateLongPresses();
            return;
        }

        libinput_dispatch(context);

        for (struct libinput_event* event; (event = libinput_get_event(context)) != nullptr;)
        {
            if (libinput_event_get_type(event) == LIBINPUT_EVENT_KEYBOARD_KEY)
            {
                struct libinput_event_keyboard* const keyevent = libinput_event_get_keyboard_event(event);
                const uint32_t keycode = libinput_event_keyboard_get_key(keyevent);
                uint8_t index;

                pthread_mutex_lock(&lock);

                switch (keycode)
                {
                case ENCODER_CLICK_START ... ENCODER_CLICK_START + NUM_ENCODERS:
                    index = keycode - ENCODER_CLICK_START;
                    if (libinput_event_keyboard_get_key_state(keyevent) == LIBINPUT_KEY_STATE_PRESSED)
                    {
                        state[index].time = get_time_ms();
                        state[index].value = kEventStatePressed;

                        if (tapTempo[index].enabled)
                            updateTapTempo(index, libinput_event_keyboard_get_time_usec(keyevent));
                    }
                    else
                    {
                        state[index].time = 0;
                        state[index].value = kEventStateReleased;
                    }
                    queueEvent(kEventTypeEncoder, state[index].value, index, 0);
                    break;

                case ENCODER_LEFT_START ... ENCODER_LEFT_START + NUM_ENCODERS:
                    index = keycode - ENCODER_LEFT_START;
                    queueEvent(kEventTypeEncoder, state[index].value, index, -1);
                    break;

                case ENCODER_RIGHT_START ... ENCODER_RIGHT_START + NUM_ENCODERS:
                    index = keycode - ENCODER_RIGHT_START;
                    queueEvent(kEventTypeEncoder, state[index].value, index, 1);
                    break;

                case FOOTSWITCH_CLICK_START ... FOOTSWITCH_CLICK_START + NUM_FOOTSWITCHES:
                    index = keycode - FOOTSWITCH_CLICK_START;
                    if (libinput_event_keyboard_get_key_state(keyevent) == LIBINPUT_KEY_STATE_PRESSED)
                    {
                        state[NUM_ENCODERS + index].time = get_time_ms();
                        state[NUM_ENCODERS + index].value = kEventStatePressed;

                        if (tapTempo[NUM_ENCODERS + index].enabled)
                            updateTapTempo(NUM_ENCODERS + index, libinput_event_keyboard_get_time_usec(keyevent));
                    }
                    else
                    {
                        state[NUM_ENCODERS + index].time = 0;
                        state[NUM_ENCODERS + index].value = kEventStateReleased;
                    }
                    queueEvent(kEventTypeFootswitch, state[NUM_ENCODERS + index].value, index, 0);
                    break;

                default:
                    printf("unused event keycode %d\n", keycode);
                    break;
                }

                pthread_mutex_unlock(&lock);
            }

            libinput_event_destroy(event);
        }

        updateLongPresses();
    }

    inline void queueEvent(EventType etype, EventState evalue, uint8_t index, int32_t value)
    {
        events.push_back({ etype, evalue, index, value });
    }

    void updateLongPresses()
    {
        // only ask for current time as needed
        uint32_t now = 0;

        pthread_mutex_lock(&lock);

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
                queueEvent(kEventTypeEncoder, state[i].value, i, 0);
            else
                queueEvent(kEventTypeFootswitch, state[i].value, i - NUM_ENCODERS, 0);
        }

        pthread_mutex_unlock(&lock);
    }

    void updateTapTempo(const uint8_t index, const uint64_t timeUs)
    {
        const uint64_t last = tapTempo[index].time;
        tapTempo[index].time = timeUs;

        if (last == 0 || timeUs <= last)
            return;

        uint32_t delta = timeUs - last;

        if (delta > EVENT_BRIDGE_TAP_TEMPO_TIMEOUT * 1000)
        {
            if (delta - EVENT_BRIDGE_TAP_TEMPO_TIMEOUT_OVERFLOW * 1000 > EVENT_BRIDGE_TAP_TEMPO_TIMEOUT * 1000)
                return;

            delta = EVENT_BRIDGE_TAP_TEMPO_TIMEOUT * 1000;
        }

        if (abs_delta(tapTempo[index].value, delta) < EVENT_BRIDGE_TAP_TEMPO_HYSTERESIS * 1000)
            tapTempo[index].value = (tapTempo[index].value * 2 + delta) / 3;
        else
            tapTempo[index].value = delta;

        tapTempo[index].updated = true;
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
