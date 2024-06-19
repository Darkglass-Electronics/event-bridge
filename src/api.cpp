// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "api.hpp"

#include <QtCore/QObject>

// --------------------------------------------------------------------------------------------------------------------

struct EventStream::Impl : QObject
{
    Impl(std::string& last_error)
        : last_error(last_error)
    {
    }

    ~Impl()
    {
        close();
    }

    void close()
    {
    }

private:
    std::string& last_error;
};

// --------------------------------------------------------------------------------------------------------------------

EventStream::EventStream() : impl(new Impl(last_error)) {}
EventStream::~EventStream() { delete impl; }

// --------------------------------------------------------------------------------------------------------------------
