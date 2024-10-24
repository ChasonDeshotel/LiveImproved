#pragma once
#include <cstdint>
#include <atomic>

class RequestIDGenerator {
public:
    static RequestIDGenerator &getInstance() {
        static RequestIDGenerator instance;
        return instance;
    }

    [[nodiscard]] uint64_t getNextID() noexcept {
        return nextID_.fetch_add(1, std::memory_order_relaxed);
    }

    RequestIDGenerator(const RequestIDGenerator &) = delete;
    RequestIDGenerator(RequestIDGenerator &&) = delete;
    RequestIDGenerator &operator=(const RequestIDGenerator &) = delete;
    RequestIDGenerator &operator=(RequestIDGenerator &&) = delete;

private:
    RequestIDGenerator() noexcept = default;
    ~RequestIDGenerator() = default;

    std::atomic<uint64_t> nextID_{1};
};
