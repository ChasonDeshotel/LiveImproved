#include <string>
#import <Cocoa/Cocoa.h>
#include "LiveObserver.h"

int main(int argc, char *argv[]) {
    printf("foo");
    LiveObserver::registerAppTermination([]() {
        printf("calling killall");
        std::string cmdStr = "killall LiveImproved";
        std::system(cmdStr.c_str());
    });

    LiveObserver::registerAppLaunch([]() {
        printf("calling open");
        execl("/Users/cdeshotel/source/ableton/LiveImproved/build/macos-cli/LiveImproved_artefacts/Debug/LiveImproved.app/Contents/MacOS/LiveImproved", "LiveImproved", nullptr);
    });

    [[NSRunLoop mainRunLoop] run];
    return 0;
}
