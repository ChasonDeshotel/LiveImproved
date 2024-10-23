#pragma once
#include <cstdint>

class RequestIDGenerator {
public:
    static RequestIDGenerator& getInstance();
    uint64_t getNextID();

    RequestIDGenerator(const RequestIDGenerator&) = delete;
    RequestIDGenerator& operator=(const RequestIDGenerator&) = delete;

    RequestIDGenerator(RequestIDGenerator&&) = delete;
    RequestIDGenerator& operator=(RequestIDGenerator&&) = delete;

private:
    RequestIDGenerator() = default;
    ~RequestIDGenerator() = default;
};
