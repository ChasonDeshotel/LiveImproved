#include <memory>

#include "LogGlobal.h"
#include "DependencyContainer.h"
#include "ILogger.h"
#include "LogHandler.h"
#include "PathFinder.h"

#if !defined(TEST_BUILD) && !defined(NO_JUCE)
#include "JUCESink.h"
#endif
#include "FileSink.h"

// NOLINTBEGIN
std::shared_ptr<ILogger> logger = nullptr;
// NOLINTEND

extern void initializeLogger() {
    if (!logger) {
        auto& container = DependencyContainer::getInstance();
        container.registerType<ILogger, LogHandler>(DependencyContainer::Lifetime::Singleton);
        logger = container.resolve<ILogger>();

        auto logHandler = dynamic_cast<LogHandler*>(logger.get());
        if (PathFinder::log()) {
            if (auto fileSink = std::make_shared<FileLogSink>(PathFinder::log().value()); fileSink->isAvailable()) {
                logHandler->addSink(fileSink);
            }
        }
        #if !defined(TEST_BUILD) && !defined(NO_JUCE)
        logHandler->addSink(std::make_shared<JUCELogSink>());
        #endif
    }
}