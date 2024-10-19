#pragma once

#include <functional>
#include <memory>
#include <string>

#include "LogLevel.h"

#include "IIPC.h"

class IPCResilienceDecorator : public IIPC {
public:
  IPCResilienceDecorator(std::function<std::shared_ptr<IIPC>()> factory);
  ~IPCResilienceDecorator() override = default;

  IPCResilienceDecorator(const IPCResilienceDecorator &) = delete;
  IPCResilienceDecorator(IPCResilienceDecorator &&) = delete;
  auto operator=(const IPCResilienceDecorator &) -> IPCResilienceDecorator & = delete;
  auto operator=(IPCResilienceDecorator &&) -> IPCResilienceDecorator & = delete;

  auto init() -> bool override;
  [[nodiscard]] auto isInitialized() const -> bool override;

  void writeRequest(const std::string &message) override;
  void writeRequest(const std::string &message,
                    ResponseCallback callback) override;

  auto readResponse(ResponseCallback callback) -> std::string override;
  void drainPipe(int fd) override;
  void closeAndDeletePipes() override;

  void stopIPC() override;

  auto checkAndReestablishConnection() -> bool;

private:
    std::shared_ptr<IIPC> instance_;
    std::function<std::shared_ptr<IIPC>()> ipcFactory_;

    template<typename Func>
    auto handleError(const std::string& operation, Func retry) -> decltype(retry());

    void logMessage(const std::string& message, LogLevel logLevel);
};
