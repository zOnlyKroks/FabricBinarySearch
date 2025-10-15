#ifndef FABRICBINARYSEARCH_CONFIG_H
#define FABRICBINARYSEARCH_CONFIG_H

#include <string>
#include <nlohmann/json.hpp>
#include <filesystem>

namespace fs = std::filesystem;
using json = nlohmann::json;

class Config {
public:
    static Config& getInstance();

    bool load();
    bool save();

    std::string getLastInstancePath() const;
    void setLastInstancePath(const std::string& path);

    std::string getLogLevel() const;
    void setLogLevel(const std::string& level);

    bool isAutoSaveEnabled() const;
    void setAutoSave(bool enabled);

    std::string getConfigPath() const;

    void reset();

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

private:
    Config();

    fs::path configFilePath;
    json configData;

    void initializeDefaults();
    fs::path getDefaultConfigPath() const;
};

#endif // FABRICBINARYSEARCH_CONFIG_H