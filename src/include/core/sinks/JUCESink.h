#pragma once
#ifndef TEST_BUILD
#include <JuceHeader.h>
#endif

#include "LogSink.h"

class JUCELogSink final : public LogSink {
public:
    explicit JUCELogSink() {}

    JUCELogSink(JUCELogSink&&) = delete;
    JUCELogSink(const JUCELogSink&) = delete;
    auto operator=(JUCELogSink&&) -> JUCELogSink& = delete;
    auto operator=(const JUCELogSink&) -> JUCELogSink& = delete;

    void write(const std::string_view formattedMessage) override {
        std::lock_guard lock(fileMutex);
        juce::Logger::writeToLog(formattedMessage.data());
    }

    auto isAvailable() const -> bool override { return true; }

    void flush() override {}

private:
    std::mutex fileMutex;
};