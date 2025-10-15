#ifndef FABRICBINARYSEARCH_PROGRESSSTATE_H
#define FABRICBINARYSEARCH_PROGRESSSTATE_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <filesystem>

namespace fs = std::filesystem;
using json = nlohmann::json;

struct SearchProgress {
    int iteration;
    std::vector<std::string> suspects;
    std::vector<std::string> innocent;
    std::vector<std::string> currentlyDisabled;
    std::vector<std::string> allMods;
    std::string modsDirectory;
    std::string timestamp;
    bool isActive;

    json toJson() const;
    static SearchProgress fromJson(const json& j);
};

class ProgressState {
public:
    static ProgressState& getInstance();

    bool saveProgress(const SearchProgress& progress);
    bool loadProgress(SearchProgress& progress);
    bool hasProgress() const;
    bool clearProgress();

    std::string getProgressPath() const;

    ProgressState(const ProgressState&) = delete;
    ProgressState& operator=(const ProgressState&) = delete;

private:
    ProgressState();

    fs::path progressFilePath;
    fs::path getDefaultProgressPath() const;
};

#endif // FABRICBINARYSEARCH_PROGRESSSTATE_H