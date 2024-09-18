#include <JuceHeader.h>
#include "ApplicationManager.h"
#include "LogHandler.h"

class JuceApp : public juce::JUCEApplication {
public:
    JuceApp() {}

    const juce::String getApplicationName() override {
        return "Live Improved";
    }

    const juce::String getApplicationVersion() override {
        return "0.0.0.1";
    }

    void initialise(const juce::String&) override {
        std::locale::global(std::locale("en_US.UTF-8"));
        ApplicationManager& appManager = ApplicationManager::getInstance();
        appManager.getLogHandler()->info("Ignition sequence started...");
        appManager.getLogHandler()->debug("int main()");

        // Block until Live is running
        // file pipes act fucky on windows
        // Live doesn't boot first
        // TODO implement monitoring to see when
        // live opens and closes instead of looping
        PID::getInstance().livePIDBlocking();
        
        appManager.init();
        appManager.getLogHandler()->debug("ApplicationManager::init() called");


        #ifndef _WIN32
			PlatformInitializer::init();
			appManager.getEventHandler()->setupQuartzEventTap();
			PlatformInitializer::run();
        #endif
    }

    void shutdown() override {
        // Clean up
        // TODO delete file pipes
    }
};
