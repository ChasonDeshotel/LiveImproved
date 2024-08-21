#ifndef PLATFORM_SPECIFIC_H
#define PLATFORM_SPECIFIC_H

#ifdef _WIN32
#include "platform/win/EventHandler.h"
#include "platform/win/IPCUtils.h"
#include "platform/win/KeySender.h"
#else
#include "platform/macos/EventHandler.h"
#include "platform/macos/IPCUtils.h"
#include "platform/macos/KeySender.h"
#endif

void initializePlatform();
void runPlatform();

#endif
