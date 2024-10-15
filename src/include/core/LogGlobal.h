#pragma once

#include <memory>

// NOLINTBEGIN
#include "ILogHandler.h"
extern void initializeLogger();
extern std::shared_ptr<ILogHandler> logger;
// NOLINTEND
