#ifndef KEY_SENDER_H
#define KEY_SENDER_H

class ApplicationManager;
class LogHandler;
class KeyMapper;

#include "Types.h"

class KeySender {
public:
    KeySender(ApplicationManager& appManager);
    ~KeySender();
    void sendKeyPress(const EKeyPress& kp);

private:
    ApplicationManager& app_;
    LogHandler* log_;
    KeyMapper* keyMapper_;

    void sendKeyDown(const EKeyPress& kp);
    void sendKeyUp(const EKeyPress& kp);
    void sendIndividualKeyPress(const EKeyPress& kp);
    void sendModifiedKeyCombo(const EKeyPress& kp);
};

#endif
