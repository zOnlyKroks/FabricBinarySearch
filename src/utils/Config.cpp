#include "Config.h"
#include "Logger.h"
#include <fstream>
#include <cstdlib>

Config::Config() {
    configFilePath = getDefaultConfigPath();
    initializeDefaults();
}

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

bool Config::load() {
    if (!fs::exists(configFilePath)) {
        LOG_INFO("Config file not found, using defaults: " + configFilePath.string());
        return save();
    }

    try {
        std::ifstream file(configFilePath);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open config file: " + configFilePath.string());
            return false;
        }

        file >> configData;
        LOG_INFO("Config loaded from: " + configFilePath.string());
        return true;

    } catch (const json::exception& e) {
        LOG_ERROR("Failed to parse config file: " + std::string(e.what()));
        initializeDefaults();
        return false;
    }
}

bool Config::save() {
    try {
        fs::path configDir = configFilePath.parent_path();
        if (!fs::exists(configDir)) {
            fs::create_directories(configDir);
        }

        std::ofstream file(configFilePath);
        if (!file.is_open()) {
            LOG_ERROR("Failed to create config file: " + configFilePath.string());
            return false;
        }

        file << configData.dump(4);
        LOG_INFO("Config saved to: " + configFilePath.string());
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save config: " + std::string(e.what()));
        return false;
    }
}

std::string Config::getLastInstancePath() const {
    return configData.value("lastInstancePath", "");
}

void Config::setLastInstancePath(const std::string& path) {
    configData["lastInstancePath"] = path;
}

std::string Config::getLogLevel() const {
    return configData.value("logLevel", "info");
}

void Config::setLogLevel(const std::string& level) {
    configData["logLevel"] = level;
}

bool Config::isAutoSaveEnabled() const {
    return configData.value("autoSaveProgress", true);
}

void Config::setAutoSave(bool enabled) {
    configData["autoSaveProgress"] = enabled;
}

std::string Config::getConfigPath() const {
    return configFilePath.string();
}

void Config::reset() {
    initializeDefaults();
    save();
}

void Config::initializeDefaults() {
    configData = {
        {"lastInstancePath", ""},
        {"logLevel", "info"},
        {"autoSaveProgress", true},
        {"version", "1.0.0"}
    };
}

fs::path Config::getDefaultConfigPath() const {
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        return fs::path(appdata) / "fabric-binary-search" / "config.json";
    }
    return fs::path("config.json");
#else
    const char* home = std::getenv("HOME");
    if (home) {
        return fs::path(home) / ".config" / "fabric-binary-search" / "config.json";
    }
    return fs::path("config.json");
#endif
}