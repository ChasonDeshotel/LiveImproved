#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <vector>

class IPC;
class Plugin;
class ResponseParser;
class LogHandler;

class PluginManager {
public:
    PluginManager(IPC& ipc_, ResponseParser& responseParser_);
    ~PluginManager();

    const std::vector<Plugin>& getPlugins() const;

    void refreshPlugins();

private:
    LogHandler& log_;
    IPC& ipc_;
    ResponseParser& responseParser_;
    std::vector<Plugin> plugins_;
};

#endif
