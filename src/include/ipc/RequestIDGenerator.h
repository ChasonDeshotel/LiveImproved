#pragma once
#include <cstdint>
#include <atomic>

class RequestIDGenerator {
public:
    static auto getInstance() -> RequestIDGenerator & {
        static RequestIDGenerator instance;
        return instance;
    }

    [[nodiscard]] auto getNextID() noexcept -> uint64_t {
        return nextID_.fetch_add(1, std::memory_order_relaxed);
    }

    RequestIDGenerator(const RequestIDGenerator &) = delete;
    RequestIDGenerator(RequestIDGenerator &&) = delete;
    auto operator=(const RequestIDGenerator &) -> RequestIDGenerator & = delete;
    auto operator=(RequestIDGenerator &&) -> RequestIDGenerator & = delete;

private:
    RequestIDGenerator() noexcept = default;
    ~RequestIDGenerator() = default;

    std::atomic<uint64_t> nextID_{1};
};
