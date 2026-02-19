#import <Cocoa/Cocoa.h>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <filesystem>
#include <libproc.h>

#include "LiveObserver.h"
#include "PID.h"

#include "LogGlobal.h"
#include "LogHandler.h"
#include "FileSink.h"

std::shared_ptr<ILogger> dLogger = std::make_shared<LogHandler>();

// fuck globals
class Daemon;
static NSMenuItem* runningStatus;
static Daemon* limDaemon;

static inline NSString* toNS(const std::string& s) {
    return [NSString stringWithUTF8String:s.c_str()];
}

NSString* getLIMPrefsDir() {
    return [@"~/Library/Preferences/LiveImproved" stringByExpandingTildeInPath];
}

NSString* getLogPath() {
    return [@"~/Library/Logs/LiveImproved" stringByExpandingTildeInPath];
}

NSString* getLivePrefsDir() {
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
    auto pid = PID::getInstance().findLivePID();
    proc_pidpath(pid, pathbuf, sizeof(pathbuf));

    std::filesystem::path execPath = pathbuf;
    auto appPath = toNS(execPath.parent_path().parent_path().parent_path());

    auto* liveBundle = [NSBundle bundleWithPath:appPath];
    NSString* version = [liveBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
    std::string shortVersion = [[version componentsSeparatedByString:@" "].firstObject UTF8String];
    auto latestVersionPathStr = std::string("~/Library/Preferences/Ableton/Live ") + shortVersion;

    auto nsPathStr = toNS(latestVersionPathStr);
    nsPathStr = [nsPathStr stringByExpandingTildeInPath];
    return nsPathStr;
}

std::string getRemoteScriptsPath() {
    NSString* pathStr = getLivePrefsDir();
    std::string pathStdStr = [pathStr UTF8String];
    NSString* cfgPath = [pathStr stringByAppendingPathComponent:@"Library.cfg"];
    NSData* data = [NSData dataWithContentsOfFile:cfgPath];
    NSXMLDocument* doc = [[NSXMLDocument alloc] initWithData:data options:0 error:nil];
    NSArray* nodes = [doc nodesForXPath:@"//UserLibrary/LibraryProject/ProjectPath/@Value" error:nil];
    std::string projectPath = [[[[nodes firstObject] stringValue] stringByAppendingPathComponent:@"User Library/Remote Scripts/LiveImproved"] UTF8String];
    return projectPath;
}

void installRemoteScriptsMaybe(bool force = false) {
    NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
    std::filesystem::path bundledRemoteScriptsPath = [[[[NSBundle mainBundle] bundlePath]
                            stringByAppendingPathComponent:@"Contents/Resources/remote-scripts"] UTF8String];

    // TODO version check to update remote scripts
    try {
        if (force) {
            std::filesystem::copy(bundledRemoteScriptsPath, getRemoteScriptsPath());
        } else {
            if (!std::filesystem::exists(getRemoteScriptsPath())) {
                std::filesystem::copy(bundledRemoteScriptsPath, getRemoteScriptsPath());
            }
        }
    } catch (const fs::filesystem_error& ex) {
        dLogger->error("error: {}", ex.what());
    }
}

void installConfigFilesMaybe(bool force = false) {
    NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
    std::filesystem::path bundledConfigPath = [[[[NSBundle mainBundle] bundlePath]
                            stringByAppendingPathComponent:@"Contents/Resources/config"] UTF8String];

    auto prefsPath = std::filesystem::path(
        [getLIMPrefsDir() UTF8String]
    );

    auto configFilePath = bundledConfigPath / "config.txt";
    auto configMenuFilePath = bundledConfigPath / "config-menu.txt";

    if (!std::filesystem::exists(configFilePath)) {
        dLogger->warn("bundled config.txt is missing");
        return;
    }

    if (!std::filesystem::exists(configMenuFilePath)) {
        dLogger->warn("bundled config-menu.txt is missing");
        return;
    }

    // force option unimplemented
    // we gentle
    try {
        if (!std::filesystem::exists(prefsPath / "config.txt")) {
            std::filesystem::copy(configFilePath, prefsPath);
        }

        if (!std::filesystem::exists(prefsPath / "config-menu.txt")) {
            std::filesystem::copy(configMenuFilePath, prefsPath);
        }
    } catch (const fs::filesystem_error& ex) {
        dLogger->error("error: {}", ex.what());
    }
}

void createDirectoriesMaybe() {
    dLogger->info("first run setup");
    auto prefsPath = std::filesystem::path(
        [getLIMPrefsDir() UTF8String]
    );
    auto logPath = std::filesystem::path([getLogPath() UTF8String]);
    std::filesystem::create_directories(prefsPath);
    std::filesystem::create_directories(logPath);
}

class Daemon {
  public:

    Daemon(NSMenuItem* statusItem) : statusItem_(statusItem) {
        createDirectoriesMaybe();

        std::filesystem::path logPath = [getLogPath() UTF8String];
        auto logFile = logPath / "daemon.txt";
        if (auto fileSink = std::make_shared<FileLogSink>(logFile); fileSink->isAvailable()) {
                dLogger->addSink(fileSink);
        }
        dLogger->info("constructor");

        installConfigFilesMaybe();
        installRemoteScriptsMaybe();

        //if (PID::getInstance().isLiveRunning()) {
        //    dLogger->info("launch because already running");
        //    launchLIM();
        //    std::this_thread::sleep_for(std::chrono::milliseconds(500));
        //}
        //registerObservers();
        //pidPoll();
    }

    void updateStatus(const NSMenuItem* item, const std::string& text) {
        auto nsText = toNS(text);
        NSString* title = [@"● " stringByAppendingString:nsText];
        [item setTitle:title];
    }

    void pidPoll() {
        dLogger->info("pid poll");
        std::thread([this](){
            while(true) {
                if (PID::getInstance().isLIMRunning()) {
                    setStatusActive();
                } else {
                    setStatusInactive();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(2500));
            }
        }).detach();

        std::thread([this](){
            while(true) {
                if (!PID::getInstance().isLiveRunning() && PID::getInstance().isLIMRunning()) {
                    for (auto pid : PID::getInstance().findLiveImproveds()) {
                        dLogger->warn("LiveImproved instance with PID {} will to now be kill.", pid);
                        kill(pid, SIGTERM);
                    }
                    for (auto pid : PID::getInstance().findLiveImproveds()) {
                        dLogger->warn("Stubborn LiveImproved instance with PID {} now is get killed but more.", pid);
                        kill(pid, SIGKILL);
                    }
                }
                if (PID::getInstance().isLiveRunning() && !PID::getInstance().isLIMRunning()) {
                    launchLIM();
                }

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }).detach();
    }

    void setStatusActive() {
        dLogger->info("set status active");
        updateStatus(statusItem_, "Active");
    }

    void setStatusInactive() {
        dLogger->info("set status inactive");
        updateStatus(statusItem_, "Inactive");
    }

    void killLIM() {
        dLogger->info("kill lim");
        std::string cmdStr = "killall LiveImproved";
        std::system(cmdStr.c_str());
        setStatusInactive();
    }

    void launchLIM() {
        dLogger->info("launch lim");
        std::lock_guard<std::mutex> lock(launchMutex_);

        if (PID::getInstance().isLIMRunning()) {
            dLogger->warn("already running");
            return;
        };
        NSTask *task = [[NSTask alloc] init];
        task.launchPath = @"/Applications/LiveImproved.app/Contents/MacOS/LiveImproved";
        [task launch];
        setStatusActive();
    }

    void registerObservers() {
        dLogger->info("register observers");
        LiveObserver::registerAppTermination([this]() {
            dLogger->info("calling killall");
            killLIM();
        });

        LiveObserver::registerAppLaunch([this]() {
            dLogger->info("calling open");
            launchLIM();
        });
    }
  private:
    NSMenuItem* statusItem_;
    std::mutex launchMutex_;
};


//NSImage* iconInactive = [NSImage imageNamed:@"StatusIconInactive"];
//NSImage* iconActive = [NSImage imageNamed:@"StatusIconActive"];
//void updateStatusIcon(NSStatusItem* topIconItem, BOOL active) {
//    topIcon.button.image = active ? iconActive : iconInactive;
//}

@interface AppDelegate : NSObject <NSApplicationDelegate>
- (void)openGitHub:(id)sender;
- (void)openConfigDir:(id)sender;
- (void)openLogDir:(id)sender;
- (void)quitApp;
- (void)applicationDidFinishLaunching:(NSNotification *)notification;
@end

@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    ::initializeLogger();
    limDaemon = new Daemon(runningStatus);
}

- (void)openGitHub:(id)sender {
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://github.com/chasondeshotel/liveimproved"]];
}
- (void)openConfigDir:(id)sender {
    [[NSWorkspace sharedWorkspace] openFile:getLIMPrefsDir()];
}
- (void)openLogDir:(id)sender {
    [[NSWorkspace sharedWorkspace] openFile:getLogPath()];
}
- (void)installRemoteScripts:(id)sender {
    installRemoteScriptsMaybe(true);
}
- (void)quitApp {
    std::system("killall LiveImproved");
    sleep(1);
    [NSApp terminate:nil];
}
@end

int main(int argc, char *argv[]) {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
    AppDelegate* delegate = [[AppDelegate alloc] init];

    NSStatusItem* topIconItem = [[NSStatusBar systemStatusBar]
        statusItemWithLength:NSVariableStatusItemLength];
    topIconItem.button.title = @"LIM";

    NSImage* icon = [NSImage imageNamed:@"StatusIcon.png"];
    [icon setTemplate:YES]; // makes it adapt to light/dark menu bar
    topIconItem.button.image = icon;

    NSMenu* menu = [[NSMenu alloc] init];

    runningStatus = [[NSMenuItem alloc] initWithTitle:@"● Inactive"
                                                    action:nil
                                                    keyEquivalent:@""];
    [runningStatus setEnabled:NO]; // greyed out, non-clickable

    NSMenuItem* githubItem = [[NSMenuItem alloc] initWithTitle:@"View Source Code"
                                                 action:@selector(openGitHub:)
                                                 keyEquivalent:@""];
    githubItem.target = delegate;

    NSMenuItem* configItem = [[NSMenuItem alloc] initWithTitle:@"Open Config Directory"
                                                 action:@selector(openConfigDir:)
                                                 keyEquivalent:@""];
    configItem.target = delegate;

    NSMenuItem* logItem = [[NSMenuItem alloc] initWithTitle:@"Open Log Directory"
                                              action:@selector(openLogDir:)
                                              keyEquivalent:@""];
    logItem.target = delegate;

    NSMenuItem* installRemoteScriptsItem = [[NSMenuItem alloc] initWithTitle:@"Reinstall MIDI Remote Scripts"
                                              action:@selector(installRemoteScripts:)
                                              keyEquivalent:@""];
    installRemoteScriptsItem.target = delegate;

    NSMenuItem* helpItem = [[NSMenuItem alloc] initWithTitle:@"Help"
                                                 action:@selector(openGitHub:)
                                                 keyEquivalent:@""];
    helpItem.target = delegate;

    NSMenuItem* quitItem = [[NSMenuItem alloc] initWithTitle:@"Quit"
                                               action:@selector(quitApp)
                                               keyEquivalent:@"q"];
    quitItem.target = delegate;

    [menu addItem:runningStatus];
    [menu addItem:[NSMenuItem separatorItem]];
    [menu addItem:githubItem];
    [menu addItem:[NSMenuItem separatorItem]];
    [menu addItem:configItem];
    [menu addItem:logItem];
    [menu addItem:[NSMenuItem separatorItem]];
    [menu addItem:installRemoteScriptsItem];
    [menu addItem:[NSMenuItem separatorItem]];
    [menu addItem:helpItem];
    [menu addItem:[NSMenuItem separatorItem]];
    [menu addItem:quitItem];

    topIconItem.menu = menu;

    [NSApp setDelegate:delegate];
    [NSApp run];

    return 0;
}