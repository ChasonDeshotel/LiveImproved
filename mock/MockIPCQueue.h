#pragma once

#include "IIPCQueue.h"
#include <vector>
#include <functional>

class MockIPCQueue : public IIPCQueue {
public:
    MockIPCQueue() = default;
    ~MockIPCQueue() override = default;

    auto init() -> ipc::QueueState override { return state_; }
    [[nodiscard]] auto isInitialized() const -> bool override { return initialized_; }

    auto createRequest(const std::string& message) -> void override {
        requests_.push_back(message);
    }

    auto createRequest(const std::string& message, ipc::ResponseCallback callback) -> void override {
        requests_.push_back(message);
        callbacks_.push_back(callback);
    }

    auto halt() -> void override { halted_ = true; }

    auto cleanUpPipes() -> void override { cleanedUp_ = true; }

    // Helper methods for testing
    void setInitialized(bool value) { initialized_ = value; }
    void setState(ipc::QueueState state) { state_ = state; }
    [[nodiscard]] const std::vector<std::string>& getRequests() const { return requests_; }
    [[nodiscard]] const std::vector<ipc::ResponseCallback>& getCallbacks() const { return callbacks_; }
    [[nodiscard]] bool isHalted() const { return halted_; }
    [[nodiscard]] bool isCleanedUp() const { return cleanedUp_; }

private:
    bool initialized_ = false;
    ipc::QueueState state_ = ipc::QueueState::Uninitialized;
    std::vector<std::string> requests_;
    std::vector<ipc::ResponseCallback> callbacks_;
    bool halted_ = false;
    bool cleanedUp_ = false;
};
