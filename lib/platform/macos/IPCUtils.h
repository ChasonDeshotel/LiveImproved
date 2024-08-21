#ifndef IPCUTILS_H
#define IPCUTILS_H

#include <iostream>
#include <map>
#include <string>

class IPCUtils {
public:
    IPCUtils();
    ~IPCUtils();

    void createPipe(const std::string& pipe_name);
    void openPipeForWrite(const std::string& pipe_name, bool non_blocking = false);
    void openPipeForRead(const std::string& pipe_name, bool non_blocking = false);

    void writeToPipe(const std::string& pipe_name, const std::string& message);
    std::string readFromPipe(const std::string& pipe_name);

private:
    std::map<std::string, int> pipes_;
};

#endif 
