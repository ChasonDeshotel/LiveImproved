#import <Cocoa/Cocoa.h>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <filesystem>
#include <libproc.h>
#include <spawn.h>
#include <signal.h>

#include "LiveObserver.h"
#include "PID.h"

#include "LogGlobal.h"
#include "LogHandler.h"
#include "FileSink.h"
#include "PathFinder.h"

std::shared_ptr<ILogger> dLogger = std::make_shared<LogHandler>();
std::atomic<bool> running_{true};

// fuck globals
class Daemon;
static NSMenuItem* runningStatus;
static Daemon* limDaemon;

static inline NSString* toNS(const std::string& s) {
    return [NSString stringWithUTF8String:s.c_str()];
}

void installRemoteScriptsMaybe(bool force = false) {
    NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
    std::filesystem::path bundledRemoteScriptsPath = [[[[NSBundle mainBundle] bundlePath]
                            stringByAppendingPathComponent:@"Contents/Resources/remote-scripts"] UTF8String];

    // TODO version check to update remote scripts
    try {
        if (force) {
            dLogger->info("force installing bundled MIDI Remote Scripts");
            std::filesystem::copy(bundledRemoteScriptsPath, PathFinder::getRemoteScriptsPath());
        } else {
            if (!std::filesystem::exists(PathFinder::getRemoteScriptsPath())) {
                dLogger->info("installing bundled MIDI Remote Scripts");
                std::filesystem::copy(bundledRemoteScriptsPath, PathFinder::getRemoteScriptsPath());
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

    auto prefsPath = PathFinder::getLIMPrefsDir();

    auto configFilePath = prefsPath / "config.yaml";
    auto configMenuFilePath = prefsPath / "config-menu.yaml";

    // force option unimplemented
    // we gentle
    try {
        if (!std::filesystem::exists(configFilePath)) {
            dLogger->info("installing bundled config.yaml");
            std::filesystem::copy(configFilePath, prefsPath);
        }

        if (!std::filesystem::exists(configMenuFilePath)) {
            dLogger->info("installing bundled config-menu.yaml");
            std::filesystem::copy(configMenuFilePath, prefsPath);
        }
    } catch (const fs::filesystem_error& ex) {
        dLogger->error("error: {}", ex.what());
    }
}

void createDirectoriesMaybe() {
    dLogger->debug("first run setup");
    auto prefsPath = std::filesystem::path(
        [PathFinder::getLIMPrefsDirNS() UTF8String]
    );
    auto logPath = std::filesystem::path([PathFinder::getLogPathNS() UTF8String]);
    std::filesystem::create_directories(prefsPath);
    std::filesystem::create_directories(logPath);
}

class Daemon {
  public:

    Daemon(NSMenuItem* statusItem) : statusItem_(statusItem) {
        createDirectoriesMaybe();

        std::filesystem::path logPath = [PathFinder::getLogPathNS() UTF8String];
        auto logFile = logPath / "daemon.txt";
        std::filesystem::remove(logFile);
        if (auto fileSink = std::make_shared<FileLogSink>(logFile); fileSink->isAvailable()) {
            dLogger->addSink(fileSink);
        }
        dLogger->setLogLevel(LogLevel::LOG_INFO);
        dLogger->debug("constructor");

        // initial setup needs the ableton live version..
        // and the easy way to get that is from a running instance
        // so we'll just wait until live is running
        while(!PID::getInstance().isLiveRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        }

        installConfigFilesMaybe();
        installRemoteScriptsMaybe();

        if (PID::getInstance().isLiveRunning()) {
            dLogger->debug("launch because already running");
            launchLIM();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        registerObservers();
        pidPoll();
    }

    void updateStatus(const NSMenuItem* item, const std::string& text) {
        auto nsText = toNS(text);
        NSString* title = [@"● " stringByAppendingString:nsText];
        [item setTitle:title];
    }

    void pidPoll() {
        dLogger->debug("pid poll");
        std::thread([this](){
            while(running_) {
                if (PID::getInstance().isLIMRunning()) {
                    setStatusActive();
                } else {
                    setStatusInactive();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(2500));
            }
        }).detach();

        std::thread([this](){
            while(running_) {
                if (!PID::getInstance().isLiveRunning() && PID::getInstance().isLIMRunning()) {
                    killLIM();
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                if (PID::getInstance().isLiveRunning() && !PID::getInstance().isLIMRunning()) {
                    launchLIM();
                }

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }).detach();
    }

    void setStatusActive() {
        dLogger->trace("set status active");
        updateStatus(statusItem_, "Active");
    }

    void setStatusInactive() {
        dLogger->trace("set status inactive");
        updateStatus(statusItem_, "Inactive");
    }

    void killLIM() {
        for (auto pid : PID::getInstance().findLiveImproveds()) {
            dLogger->warn("LiveImproved instance with PID {} will to now be kill.", pid);
            kill(pid, SIGTERM);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        for (auto pid : PID::getInstance().findLiveImproveds()) {
            dLogger->warn("Stubborn LiveImproved instance with PID {} now is get killed but more.", pid);
            kill(pid, SIGKILL);
        }
        setStatusInactive();
    }

    void launchLIM() {
        dLogger->debug("launch lim");
        std::lock_guard<std::mutex> lock(launchMutex_);

        if (PID::getInstance().isLIMRunning()) {
            dLogger->warn("already running");
            return;
        };

        auto* limBinaryPath = [[[[NSBundle mainBundle] bundlePath]
                            stringByAppendingPathComponent:@"Contents/MacOS/LiveImproved"] UTF8String];

        posix_spawnattr_t attr;
        posix_spawnattr_init(&attr);
        posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSID);

        char* const argv[] = { (char*)"LiveImproved", nullptr };
        char* const envp[] = { nullptr };

        pid_t pid;
        posix_spawn(&pid, limBinaryPath, nullptr, &attr, argv, envp);
        posix_spawnattr_destroy(&attr);
        setStatusActive();
    }

    void registerObservers() {
        dLogger->debug("register observers");
        LiveObserver::registerAppTermination([this]() {
            dLogger->debug("calling killall");
            killLIM();
        });

        LiveObserver::registerAppLaunch([this]() {
            dLogger->debug("calling open");
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
    [[NSWorkspace sharedWorkspace] openFile:PathFinder::getLIMPrefsDirNS()];
}
- (void)openLogDir:(id)sender {
    [[NSWorkspace sharedWorkspace] openFile:PathFinder::getLogPathNS()];
}
- (void)installRemoteScripts:(id)sender {
    installRemoteScriptsMaybe(true);
}
- (void)quitApp {
    running_ = false;
    limDaemon->killLIM();
    [NSApp terminate:nil];
}
@end

int main(int argc, char *argv[]) {
    signal(SIGCHLD, SIG_IGN);

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