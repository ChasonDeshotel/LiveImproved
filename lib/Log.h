#ifndef LOG_H
#define LOG_H

#include <string>
#include <fstream>

class Log {
public:
    static void logToFile(const std::string &message);
private:
    static const char *logFilePath;
};

#endif
