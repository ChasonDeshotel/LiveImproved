#ifndef intercept_keys_h
#define intercept_keys_h

#import <Cocoa/Cocoa.h>

#ifdef __cplusplus
#include <string>
extern "C" {
#endif

void logToFile(const std::string &message);

#ifdef __cplusplus
}
#endif

#include <ApplicationServices/ApplicationServices.h>

static pid_t abletonLivePID = 0;
static const char *logFilePath = "/Users/cdeshotel/Scripts/Ableton/InterceptKeys/log.txt";

CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);

void setAbletonLivePID();
void setupQuartzEventTap();
void introspect();

#endif
