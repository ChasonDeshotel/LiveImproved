#ifndef PLATFORM_INITIALIZER_H
#define PLATFORM_INITIALIZER_H

class PlatformInitializer {
public:
    static void init();
    static void run();

private:
    PlatformInitializer()  = default;
    ~PlatformInitializer() = default;
};

#endif
