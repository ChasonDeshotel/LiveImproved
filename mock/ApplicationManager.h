#include "LogHandler.h"

class ApplicationManager {
public:
    ApplicationManager(); 

    LogHandler* getLogHandler();

private:
    LogHandler& mockLogHandler;  // Reference to the singleton instance
};
