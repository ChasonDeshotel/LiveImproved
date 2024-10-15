#pragma once

#include <string>

#include "Types.h"

class KeyMapper {
public:
  KeyMapper();
  KeyMapper(const KeyMapper &) = default;
  KeyMapper(KeyMapper &&) = delete;
  auto operator=(const KeyMapper &) -> KeyMapper & = default;
  auto operator=(KeyMapper &&) -> KeyMapper & = delete;
  ~KeyMapper();

  // Method to process a keypress string and return a KeyPress object
  auto processKeyPress(const std::string &keypress) -> EKeyPress;
  auto EKeyPressToString(const EKeyPress &keypress) -> std::string;

  [[nodiscard]] auto isValid() const -> bool;
  [[nodiscard]] auto getKeyPress() const -> const EKeyPress&;

private:
    bool valid;
    EKeyPress keypress;

    [[nodiscard]] auto validateHotkey(const std::string& keypress) const -> bool;
    [[nodiscard]] auto parseKeyPress(const std::string& keypress) const -> EKeyPress;
    [[nodiscard]] auto buildRegexPattern() const -> std::string;
    [[nodiscard]] auto toLowerCase(const std::string& input) const -> std::string;
};
