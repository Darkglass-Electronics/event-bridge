// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "event-bridge.hpp"
#include "websocket.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtWebSockets/QWebSocket>

#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

// --------------------------------------------------------------------------------------------------------------------
// default configuration

#ifndef NUM_ENCODERS
#define NUM_ENCODERS 6
#endif

#ifndef NUM_FOOTSWITCHES
#define NUM_FOOTSWITCHES 3
#endif

#ifndef NUM_KNOBS
#define NUM_KNOBS 0
#endif

#ifndef NUM_LEDS
#define NUM_LEDS 0
#endif

// --------------------------------------------------------------------------------------------------------------------
// utility function that copies nested objects without deleting old values

static void copyJsonObjectValue(QJsonObject& dst, const QJsonObject& src)
{
    for (const QString& key : src.keys())
    {
        const QJsonValue& value(src[key]);

        if (value.isObject())
        {
            QJsonObject obj;
            if (dst.contains(key))
                obj = dst[key].toObject();

            copyJsonObjectValue(obj, src[key].toObject());

            dst[key] = obj;
        }
        else
        {
            dst[key] = src[key];
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

struct WebSocketEventBridge : QObject,
                              EventBridge::Callbacks,
                              WebSocketServer::Callbacks
{
    EventBridge bridge;
    WebSocketServer wsServer;
    bool ok = false;
    bool verboseLogs = false;
    int timerId = 0;

    // keep current state in memory
    QJsonObject stateJson;
    struct {
        struct {
        } encoders[NUM_ENCODERS];
        struct {
        } footswitches[NUM_FOOTSWITCHES];
        struct {
        } knobs[NUM_KNOBS];
        struct {
        } leds[NUM_LEDS];
    } current;

    WebSocketEventBridge()
        : bridge(this),
          wsServer(this)
    {
        if (! bridge.last_error.empty())
        {
            fprintf(stderr, "Failed to initialize event stream connection: %s\n", bridge.last_error.c_str());
            return;
        }

        if (! wsServer.last_error.empty())
        {
            fprintf(stderr, "Failed to initialize websocket server: %s\n", wsServer.last_error.c_str());
            return;
        }

        if (! wsServer.listen(13372))
        {
            fprintf(stderr, "Failed to start websocket server: %s\n", wsServer.last_error.c_str());
            return;
        }

        if (const char* const log = std::getenv("MOD_LOG"))
        {
            if (std::atoi(log) != 0)
                verboseLogs = true;
        }

        ok = true;
        stateJson["type"] = "state";

        timerId = startTimer(50);
    }

    // handle new websocket connection
    // this will send the current state to the socket client, along with the list of plugins and categories
    void newWebSocketConnection(QWebSocket* const ws) override
    {
        if (! stateJson.contains("encoders"))
        {
            // TODO fetch and init encoders
        }

        if (! stateJson.contains("footswitches"))
        {
            // TODO fetch and init footswitches
        }

        if (! stateJson.contains("knobs"))
        {
            // TODO fetch and init knobs
        }

        if (! stateJson.contains("leds"))
        {
            // TODO fetch and init leds
        }

        ws->sendTextMessage(QJsonDocument(stateJson).toJson(QJsonDocument::Compact));
    }

    // websocket message received, typically to indicate state changes
    void messageReceived(const QString& msg) override
    {
        const QJsonObject msgObj = QJsonDocument::fromJson(msg.toUtf8()).object();

        if (msgObj["type"] == "state")
        {
            QJsonObject stateObj;
            if (stateJson.contains("state"))
                stateObj = stateJson["state"].toObject();

            const QJsonObject msgStateObj(msgObj["state"].toObject());
            copyJsonObjectValue(stateObj, msgStateObj);

            stateJson["state"] = stateObj;

            if (verboseLogs)
            {
                puts(QJsonDocument(msgStateObj).toJson().constData());
            }

            handleStateChanges(msgStateObj);
        }
    }

    void handleStateChanges(const QJsonObject& stateObj)
    {
    }

private:
    void eventReceived(EventType etype, uint8_t index, int16_t value) override
    {
        printf("eventReceived %d:%s, %u, %d\n", etype, EventTypeStr(etype), index, value);
    }

    void timerEvent(QTimerEvent* const event) override
    {
        if (event->timerId() == timerId)
            bridge.poll();

        QObject::timerEvent(event);
    }
};

// --------------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("event-bridge");
    app.setApplicationVersion("0.0.1");
    app.setOrganizationName("Darkglass");

    WebSocketEventBridge bridge;

#ifdef HAVE_SYSTEMD
    if (bridge.ok)
        sd_notify(0, "READY=1");
#endif

    return bridge.ok ? app.exec() : 1;
}

// --------------------------------------------------------------------------------------------------------------------
