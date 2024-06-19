# event-stream

This is a little project for sending and receiving Linux-related events,
either through an API via shared library or through a websocket.

event-stream uses `/websocket` entry point on port 13372.
Connecting clients will receive the current state as json with this data model:

```
{
    "type": "state",
    "encoders": {
        "1": {
            "name": "Encoder 1",
            "value": 0
        },
        "2": {
            "name": "Encoder 2",
            "value": 0
        },
        "3": {
            "name": "Encoder 3",
            "value": 0
        },
        "4": {
            "name": "Encoder 4",
            "value": 0
        },
        "5": {
            "name": "Encoder 5",
            "value": 0
        },
        "6": {
            "name": "Encoder 6",
            "value": 0
        }
    },
    "footswitches": {
        "1": {
            "name": "Foot A",
            "value": false
        },
        "2": {
            "name": "Foot B",
            "value": false
        },
        "3": {
            "name": "Foot C",
            "value": false
        }
    },
    "knobs": {
    },
    "leds": {
    }
}
```

With subsequent events sent in partial mode.
That is to say, only the values that changed are sent, like so:

```
{
    "type": "state",
    "footswitches": {
        "1": {
            "value": true
        }
    }
}
```

These actuator types can be changed from the API/websocket server side:

- encoders
- footswitches
- knobs (TBD)

And these actuator types can be changed from the API/websocket client side:

- leds (TBD)

All actuators types use `value` as current value with the exception of "encoders".

For encoders their value is always 0, as they do not have a valid range (they can be endlessly rotated).
To make this work this separate data model is used for encoder events:

```
{
    "type": "encoder-rotation",
    "id": "1",
    "value": 1
}
```

In here the value is positive to indicate clock-wise rotation, and negative to indicate counter-clock-wise rotation.
The absolute value can be bigger than 1 to indicate very fast rotations.

## Building

This project uses cmake.
You can configure and build it in release mode by using:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The build dependencies are:

- cmake >= 3.15
- libinput
- Qt with QtWebsockets (either Qt5 or Qt6)
- systemd (optional, enables "notify" systemd event)
