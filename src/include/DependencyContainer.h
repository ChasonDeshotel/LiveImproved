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

    static DependencyContainer& getInstance() {
        static DependencyContainer instance;
        return instance;
    }

    // Delete copy and move constructors and assign operators
    DependencyContainer(DependencyContainer const&) = delete;
    DependencyContainer(DependencyContainer&&) = delete;
    DependencyContainer& operator=(DependencyContainer const&) = delete;
    DependencyContainer& operator=(DependencyContainer &&) = delete;


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

    template<typename Interface>
    void registerFactory(std::function<std::shared_ptr<Interface>(DependencyContainer&)> factory, Lifetime lifetime = Lifetime::Transient) {
        factories_[typeid(Interface)] = [this, factory, lifetime](const std::type_info& type) -> std::shared_ptr<void> {
            if (lifetime == Lifetime::Singleton) {
                auto it = singletons_.find(type);
                if (it != singletons_.end()) {
                    return it->second;
                }
            }
            auto instance = factory(*this);
            if (lifetime == Lifetime::Singleton) {
                singletons_[type] = instance;
            }
            return instance;
        };
    }

    template<typename T>
    std::shared_ptr<T> resolve() {
        return resolveImpl<T>(std::vector<std::type_index>());
    }

private:
    template<typename T>
    std::shared_ptr<T> resolveImpl(std::vector<std::type_index> resolutionStack) {
        auto typeIndex = std::type_index(typeid(T));
        if (std::find(resolutionStack.begin(), resolutionStack.end(), typeIndex) != resolutionStack.end()) {
            throw std::runtime_error("Circular dependency detected: " + std::string(typeid(T).name()));
        }

        auto it = factories_.find(typeIndex);
        if (it == factories_.end()) {
            throw std::runtime_error("Type not registered: " + std::string(typeid(T).name()));
        }

        resolutionStack.push_back(typeIndex);
        return std::static_pointer_cast<T>(it->second(typeid(T)));
    }

private:
    DependencyContainer() = default;

    template<typename T>
    std::shared_ptr<T> resolveArg() {
        return resolveImpl<T>(std::vector<std::type_index>());
    }

    std::unordered_map<
        std::type_index,
        std::function<std::shared_ptr<void>(const std::type_info&)>
    > factories_;

    std::unordered_map<std::type_index, std::shared_ptr<void>> singletons_;
};
