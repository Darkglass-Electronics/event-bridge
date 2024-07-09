// SPDX-FileCopyrightText: 2024 Filipe Coelho <falktx@darkglass.com>
// SPDX-License-Identifier: ISC

#pragma once

struct EventInput;
struct EventOutput;

EventInput* createNewInput_GPIO(int gpio, int index);
EventOutput* createNewOutput_GPIO(int gpio);

#ifdef HAVE_LIBINPUT
EventInput* createNewInput_LibInput();
#endif
