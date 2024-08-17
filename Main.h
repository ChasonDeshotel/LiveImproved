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

static pid_t abletonLivePID = 0;

static const char *logFilePath = "/Users/cdeshotel/Scripts/Ableton/InterceptKeys/log.txt";

void setAbletonLivePID();

#include <ApplicationServices/ApplicationServices.h>

// Declare the event tap callback function
CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);

// Declare the function to set up the event tap
void setupQuartzEventTap();

void introspect();

#endif
