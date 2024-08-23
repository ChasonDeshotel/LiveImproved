#ifndef PLATFORM_DEPENDENT_H
#define PLATFORM_DEPENDENT_H

#ifdef _WIN32
// do includes for win
#else
#include "platform/macos/IPC.h"
#include "platform/macos/PID.h"
#include "platform/macos/EventHandler.h"
#include "platform/macos/KeySender.h"
#endif

#endif
