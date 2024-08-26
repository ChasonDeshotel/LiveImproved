#ifndef PLATFORM_DEPENDENT_H
#define PLATFORM_DEPENDENT_H

#ifdef _WIN32
// do includes for win
#else
#include "GUISearchBox.h"
#include "IPC.h"
#include "PID.h"
#include "EventHandler.h"
#include "KeySender.h"
#endif

#endif
