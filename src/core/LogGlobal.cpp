#include <memory>

#include "LogGlobal.h"
#include "DependencyContainer.h"
#include "ILogHandler.h"
#include "LogHandler.h"

// NOLINTBEGIN
std::shared_ptr<ILogHandler> logger = nullptr;
// NOLINTEND

extern void initializeLogger() {
    if (!logger) {
        auto& container = DependencyContainer::getInstance();
        container.registerType<ILogHandler, LogHandler>(DependencyContainer::Lifetime::Singleton);
        logger = container.resolve<ILogHandler>();
        logger->setLogLevel(LogLevel::LOG_DEBUG);
    }
}
