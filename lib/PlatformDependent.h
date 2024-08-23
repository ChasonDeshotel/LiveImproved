#ifndef PLATFORM_DEPENDENT_H
#define PLATFORM_DEPENDENT_H

#ifdef _WIN32
#include "platform/win/EventHandler.h"
//#include "platform/win/IPCManager.h"
#include "platform/win/KeySender.h"
#else
#include "platform/macos/PID.h"
#include "platform/macos/EventHandler.h"
//#include "platform/macos/IPCManager.h"
#include "platform/macos/KeySender.h"
#endif

#endif
