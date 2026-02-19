#import <ApplicationServices/ApplicationServices.h>
#import <CoreFoundation/CoreFoundation.h>
#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#import <functional>
#import <string>
#include "LiveObserver.h"

void LiveObserver::registerAppLaunch(std::function<void()> onLaunchCallback) {
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserverForName:NSWorkspaceDidLaunchApplicationNotification
                                                                    object:nil
                                                                     queue:nil
                                                                usingBlock:^(NSNotification *notification) {
        NSDictionary *userInfo = [notification userInfo];
        NSRunningApplication *launchedApp = [userInfo objectForKey:NSWorkspaceApplicationKey];

        pid_t launchedPID = [launchedApp processIdentifier];
        NSString *bundleID = [launchedApp bundleIdentifier];

        if ([bundleID isEqualToString:@"com.ableton.live"]) {
            printf("Target app launched with PID: %s\n", std::to_string(launchedPID).c_str());
            if (onLaunchCallback) {
                onLaunchCallback();
            }
        }
    }];
}

void LiveObserver::registerAppTermination(std::function<void()> onTerminationCallback) {
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserverForName:NSWorkspaceDidTerminateApplicationNotification
                                                                    object:nil
                                                                     queue:nil
                                                                usingBlock:^(NSNotification *notification) {
        NSDictionary *userInfo = [notification userInfo];
        NSRunningApplication *terminatedApp = [userInfo objectForKey:NSWorkspaceApplicationKey];
        NSString *terminatedBundleID = [terminatedApp bundleIdentifier];

        printf("Termination detected for bundle ID: %s\n", std::string([terminatedBundleID UTF8String]).c_str());

        NSString *targetBundleID = @"com.ableton.live";
        if ([terminatedBundleID isEqualToString:targetBundleID]) {
            printf("Target application with bundle ID %s has been terminated\n", std::string([terminatedBundleID UTF8String]).c_str());
            if (onTerminationCallback) {
                onTerminationCallback();
            }
        }
    }];
}
