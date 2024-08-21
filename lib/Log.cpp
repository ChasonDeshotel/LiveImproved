#include "Log.h"

const char *Log::logFilePath = "/path/to/log.txt";

void Log::logToFile(const std::string &message) {
    std::ofstream logfile;
    logfile.open("/Users/cdeshotel/Scripts/Ableton/InterceptKeys/log.txt", std::ios_base::app);
    if (logfile.is_open()) {
        logfile << message << std::endl;
        logfile.close();
    }
}
