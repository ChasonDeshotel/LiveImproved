#pragma once

class PlatformInitializer {
public:
    static void init();
    static void run();

private:
    PlatformInitializer()  = default;
    ~PlatformInitializer() = default;
};
