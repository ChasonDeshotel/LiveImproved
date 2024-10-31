#pragma once

#include "IWindow.h"
#include <gmock/gmock.h>

class MockWindow : public IWindow {
public:
    MOCK_METHOD(void, open, (), (override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(void*, getWindowHandle, (), (const, override));
};
