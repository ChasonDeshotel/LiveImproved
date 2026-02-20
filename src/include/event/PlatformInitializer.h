#pragma once

class PlatformInitializer {
public:
  PlatformInitializer(const PlatformInitializer &) = default;
  PlatformInitializer(PlatformInitializer &&) = delete;
  auto operator=(const PlatformInitializer &) -> PlatformInitializer & = default;
  auto operator=(PlatformInitializer &&) -> PlatformInitializer & = delete;
  static bool checkPrivileges();
  static void init();
  static void run();
  static void stop();

private:
    PlatformInitializer()  = default;
    ~PlatformInitializer() = default;
};