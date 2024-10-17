#pragma once

#include <windows.h>
#include <iostream>
#include <map>
#include <string>
#include <functional>

#include "IIPCCore.h"

class IPCCore : public IIPCCore {
public:
  IPCCore();
  ~IPCCore() override;

  IPCCore(const IPCCore &) = default;
  IPCCore(IPCCore &&) = delete;
  IPCCore &operator=(const IPCCore &) = default;
  IPCCore &operator=(IPCCore &&) = delete;

  auto init() -> bool override;
  auto isInitialized() const -> bool override;

  auto writeRequest(const std::string& message, ResponseCallback callback) -> void override;
  auto readResponse(ResponseCallback callback) -> std::string override;
  bool initReadWithEventLoop(std::function<void(const std::string &)> callback);
  void drainPipe(HANDLE pipe);

private:
    std::string readResponseInternal(HANDLE pipe);

    std::string requestPipePath = "\\\\.\\pipe\\lim_request";
    std::string responsePipePath = "\\\\.\\pipe\\lim_response";

    std::map<std::string, HANDLE> pipes_;

    void removePipeIfExists(const std::string& pipe_name);

    bool createPipe(const std::string& pipe_name);
    bool createWritePipe(const std::string& pipe_name);
    bool createReadPipe(const std::string& pipe_name);
    bool connectToPipe(const std::string& pipe_name);
    bool openPipeForWrite(const std::string& pipe_name);
    bool openPipeForRead(const std::string& pipe_name);

    bool writeToPipe(HANDLE pipe, const std::string& message);
    std::string readFromPipe(HANDLE pipe);
};