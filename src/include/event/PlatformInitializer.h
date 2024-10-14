#pragma once

class PlatformInitializer {
public:
  PlatformInitializer(const PlatformInitializer &) = default;
  PlatformInitializer(PlatformInitializer &&) = delete;
  PlatformInitializer &operator=(const PlatformInitializer &) = default;
  PlatformInitializer &operator=(PlatformInitializer &&) = delete;
  static void init();
  static void run();

private:
    PlatformInitializer()  = default;
    ~PlatformInitializer() = default;
};
