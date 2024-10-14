#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>

class Scope {
public:
    template<typename T>
    std::shared_ptr<T> resolve(const std::function<std::shared_ptr<T>()>& factory) {
        auto it = instances_.find(typeid(T));
        if (it != instances_.end()) {
            return std::static_pointer_cast<T>(it->second);
        }
        auto instance = factory();
        instances_[typeid(T)] = instance;
        return instance;
    }

private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> instances_;
};

class DependencyContainer {
public:
    DependencyContainer() = default;
    ~DependencyContainer() = default;

    DependencyContainer(const DependencyContainer&) = delete;
    DependencyContainer& operator=(const DependencyContainer&) = delete;
    DependencyContainer(DependencyContainer&&) = delete;
    DependencyContainer& operator=(DependencyContainer&&) = delete;

    enum class Lifetime {
        Transient,
        Scoped,
        Singleton
    };

    static auto getInstance() -> DependencyContainer& {
        static DependencyContainer instance;
        return instance;
    }

    template<typename Interface, typename Impl, typename... Args>
    void registerType(Lifetime lifetime = Lifetime::Transient) {
        factories_[typeid(Interface)] = [this, lifetime](const std::type_info& type) -> std::shared_ptr<void> {
            if (lifetime == Lifetime::Singleton) {
                auto it = singletons_.find(type);
                if (it != singletons_.end()) {
                    return it->second;
                }
            }

            auto createInstance = [this]() {
                return std::make_shared<Impl>(resolveArg<Args>()...);
            };

            if (lifetime == Lifetime::Scoped && currentScope_) {
                return currentScope_->resolve<Interface>(createInstance);
            }

            auto instance = createInstance();

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

            auto createInstance = [this, factory]() {
                return factory(*this);
            };

            if (lifetime == Lifetime::Scoped && currentScope_) {
                return currentScope_->resolve<Interface>(createInstance);
            }

            auto instance = createInstance();

            if (lifetime == Lifetime::Singleton) {
                singletons_[type] = instance;
            }

            return instance;
        };
    }

    template<typename T>
    auto resolve() -> std::shared_ptr<T> {
        return resolveImpl<T>(std::vector<std::type_index>());
    }

    auto createScope() -> Scope {
        return {};
    }

    void beginScope(Scope& scope) {
        currentScope_ = &scope;
    }

    void endScope() {
        currentScope_ = nullptr;
    }

private:
    template<typename T>
    auto resolveImpl(std::vector<std::type_index> resolutionStack) -> std::shared_ptr<T> {
        auto typeIndex = std::type_index(typeid(T));
        if (std::find(resolutionStack.begin(), resolutionStack.end(), typeIndex) != resolutionStack.end()) {
            throw std::runtime_error(std::string("Circular dependency detected: ") + typeid(T).name());
        }

        resolutionStack.push_back(typeIndex);

        auto it = factories_.find(typeIndex);
        if (it == factories_.end()) {
            throw std::runtime_error(std::string("Type not registered: ") + typeid(T).name());
        }

        // get the resolved instance
        return std::static_pointer_cast<T>(it->second(typeid(T)));

        resolutionStack.pop_back();
    }

    template<typename T>
    auto resolveArg() -> std::shared_ptr<T> {
        return resolveImpl<T>(std::vector<std::type_index>());
    }

    std::unordered_map<
        std::type_index,
        std::function<std::shared_ptr<void>(const std::type_info&)>
    > factories_;

    std::unordered_map<std::type_index, std::shared_ptr<void>> singletons_;
    Scope* currentScope_ = nullptr;
};
