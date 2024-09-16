#ifndef KEY_MAPPER_H
#define KEY_MAPPER_H

#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <algorithm>

class LogHandler;
class EKeyPress;

class KeyMapper {
public:
  KeyMapper();
  ~KeyMapper();

  // Method to process a keypress string and return a KeyPress object
  EKeyPress processKeyPress(const std::string &keypress);
  std::string EKeyPressToString(const EKeyPress &keypress);

  bool isValid() const;
  const EKeyPress &getKeyPress() const;

private:
    LogHandler* log_;
    bool valid;
    EKeyPress keypress;

    bool validateHotkey(const std::string& keypress) const;
    EKeyPress parseKeyPress(const std::string& keypress) const;
    std::string buildRegexPattern() const;
    std::string toLowerCase(const std::string& input) const;
};


#endif 
