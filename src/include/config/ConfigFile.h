#pragma once

#include <fstream>
#include <yaml-cpp/yaml.h>

class Config {
public:
    explicit Config(std::string filename) : filename_(std::move(filename)) {}

    template<typename T>
    auto read(const std::string &key) const -> T {
        YAML::Node config = YAML::LoadFile(filename_);
        if (config[key]) {
            return config[key].as<T>();
        } else {
            throw std::runtime_error("Key not found in config: " + key);
        }
    }

    template<typename T>
    void write(const std::string &key, const T &value) {
        YAML::Node config;
        // Load existing configuration if the file exists
        std::ifstream infile(filename_);
        if (infile.good()) {
            config = YAML::LoadFile(filename_);
        }
        config[key] = value;
        std::ofstream fout(filename_);
        fout << config;
    }

private:
    std::string filename_;
};
