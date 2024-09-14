#include <JuceHeader.h>
#include "ApplicationManager.h"

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
        
        appManager.init();
        appManager.getLogHandler()->debug("ApplicationManager::init() called");

        // Block until Live is running
        PID::getInstance().livePIDBlocking();

        PlatformInitializer::init();
        appManager.getEventHandler()->setupQuartzEventTap();
        PlatformInitializer::run();
    }

    void shutdown() override {
        // Clean up
    }
};
