#include <JuceHeader.h>
#include "ApplicationManager.h"

class JuceApp : public juce::JUCEApplication {
public:
    JuceApp() {}

    const juce::String getApplicationName() override {
        return "My JUCE Application";
    }

    const juce::String getApplicationVersion() override {
        return "1.0";
    }

    void initialise(const juce::String&) override {
        ApplicationManager& appManager = ApplicationManager::getInstance();
        appManager.getLogHandler()->info("Application started");
        appManager.getLogHandler()->info("int main()");
        
        appManager.init();
        appManager.getLogHandler()->info("ApplicationManager::init() called");

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
