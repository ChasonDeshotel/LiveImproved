#pragma once

#include <memory>
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <stdexcept>
#include <sstream>

class DependencyContainer {
public:
    enum class Lifetime {
        Transient,
        Singleton
    };

    template<typename Interface, typename Impl, typename... Args>
    void registerType(Lifetime lifetime = Lifetime::Transient) {
        factories_[typeid(Interface)] = [this, lifetime](const std::type_info& type) -> std::shared_ptr<void> {
            if (lifetime == Lifetime::Singleton) {
                auto it = singletons_.find(type);
                if (it != singletons_.end()) {
                    return it->second;
                }
            }

            auto instance = std::make_shared<Impl>(resolveArg<Args>()...);

            if (lifetime == Lifetime::Singleton) {
                singletons_[type] = instance;
            }

            return instance;
        };
    }

    template<typename T>
    std::shared_ptr<T> resolve() {
        auto it = factories_.find(typeid(T));
        if (it == factories_.end()) {
            throw std::runtime_error("Type not registered: " + std::string(typeid(T).name()));
        }
        return std::static_pointer_cast<T>(it->second(typeid(T)));
    }

private:
    template<typename T>
    std::shared_ptr<T> resolveArg() {
        return resolve<T>();
    }

    std::unordered_map<
        std::type_index, 
        std::function<std::shared_ptr<void>(const std::type_info&)>
    > factories_;

    std::unordered_map<std::type_index, std::shared_ptr<void>> singletons_;
};
