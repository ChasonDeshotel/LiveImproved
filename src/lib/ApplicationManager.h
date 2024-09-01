#ifndef APPLICATION_MANAGER_H
#define APPLICATION_MANAGER_H

#include <vector>
#include <numeric>

#include "PlatformDependent.h"
#include "ActionHandler.h"
#include "ConfigManager.h"
#include "LogHandler.h"
#include "ResponseParser.h"
#include "EventHandler.h"
#include "SearchBox.h"
#include "DragTarget.h"

class ApplicationManager {
public:
    static ApplicationManager& getInstance() {
        static ApplicationManager instance;
        return instance;
    }

    void init();

    LogHandler* getLogHandler();

    ConfigManager* getConfigManager();
    EventHandler* getEventHandler();
    ActionHandler* getActionHandler();
    KeySender* getKeySender();
    IPC* getIPC();
    GUISearchBox* getGUISearchBox();
    DragTarget* getDragTarget();
    ResponseParser* getResponseParser();

    std::string pluginCacheStr;
    std::string getPluginsAsStr() const;
    std::vector<Plugin> getPlugins();

    void refreshPluginCache();

private:
    ApplicationManager();

    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;

    IPC* ipc_ = nullptr;
    ConfigManager* configManager_ = nullptr;
    EventHandler* eventHandler_ = nullptr;
    ActionHandler* actionHandler_ = nullptr;
    KeySender* keySender_ = nullptr;
    GUISearchBox* guiSearchBox_;
    DragTarget* dragTarget_;
    ResponseParser* responseParser_ = nullptr;

    LogHandler* logHandler_;

    std::vector<Plugin> plugins_;

    std::vector<std::string> splitStringInPlace(std::string& str, char delimiter);

};

#endif
