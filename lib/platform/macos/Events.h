#ifndef EVENTS_H
#define EVENTS_H

CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);

void setupQuartzEventTap();

#endif
