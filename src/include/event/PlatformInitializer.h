#pragma once

class PlatformInitializer {
public:
  PlatformInitializer(const PlatformInitializer &) = default;
  PlatformInitializer(PlatformInitializer &&) = delete;
  auto operator=(const PlatformInitializer &) -> PlatformInitializer & = default;
  auto operator=(PlatformInitializer &&) -> PlatformInitializer & = delete;
  static void init();
  static void run();

private:
    PlatformInitializer()  = default;
    ~PlatformInitializer() = default;
};
