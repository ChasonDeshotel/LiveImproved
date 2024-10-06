#pragma once

class ILogHandler;

class ILiveInterface {
public:
    virtual ~ILiveInterface() = default;

    virtual bool focusWindow(int windowID) = 0;
    virtual int getMostRecentFloatingWindow() = 0;
};
