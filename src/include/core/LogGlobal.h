#pragma once

#include <memory>

// NOLINTBEGIN
#include "ILogger.h"
extern void initializeLogger();
extern std::shared_ptr<ILogger> logger;
// NOLINTEND