#include "ProgressState.h"
#include "Logger.h"
#include <fstream>
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <sstream>

json SearchProgress::toJson() const {
    return {
        {"iteration", iteration},
        {"suspects", suspects},
        {"innocent", innocent},
        {"currentlyDisabled", currentlyDisabled},
        {"allMods", allMods},
        {"modsDirectory", modsDirectory},
        {"timestamp", timestamp},
        {"isActive", isActive}
    };
}

SearchProgress SearchProgress::fromJson(const json& j) {
    SearchProgress progress;
    progress.iteration = j.value("iteration", 0);
    progress.suspects = j.value("suspects", std::vector<std::string>{});
    progress.innocent = j.value("innocent", std::vector<std::string>{});
    progress.currentlyDisabled = j.value("currentlyDisabled", std::vector<std::string>{});
    progress.allMods = j.value("allMods", std::vector<std::string>{});
    progress.modsDirectory = j.value("modsDirectory", "");
    progress.timestamp = j.value("timestamp", "");
    progress.isActive = j.value("isActive", false);
    return progress;
}

ProgressState::ProgressState() {
    progressFilePath = getDefaultProgressPath();
}

ProgressState& ProgressState::getInstance() {
    static ProgressState instance;
    return instance;
}

bool ProgressState::saveProgress(const SearchProgress& progress) {
    try {
        fs::path progressDir = progressFilePath.parent_path();
        if (!fs::exists(progressDir)) {
            fs::create_directories(progressDir);
        }

        std::ofstream file(progressFilePath);
        if (!file.is_open()) {
            LOG_ERROR("Failed to create progress file: " + progressFilePath.string());
            return false;
        }

        json progressJson = progress.toJson();
        file << progressJson.dump(4);

        LOG_INFO("Progress saved to: " + progressFilePath.string());
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save progress: " + std::string(e.what()));
        return false;
    }
}

bool ProgressState::loadProgress(SearchProgress& progress) {
    if (!fs::exists(progressFilePath)) {
        LOG_DEBUG("No progress file found: " + progressFilePath.string());
        return false;
    }

    try {
        std::ifstream file(progressFilePath);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open progress file: " + progressFilePath.string());
            return false;
        }

        json progressJson;
        file >> progressJson;

        progress = SearchProgress::fromJson(progressJson);
        LOG_INFO("Progress loaded from: " + progressFilePath.string());
        return true;

    } catch (const json::exception& e) {
        LOG_ERROR("Failed to parse progress file: " + std::string(e.what()));
        return false;
    }
}

bool ProgressState::hasProgress() const {
    if (!fs::exists(progressFilePath)) {
        return false;
    }

    try {
        std::ifstream file(progressFilePath);
        if (!file.is_open()) {
            return false;
        }

        json progressJson;
        file >> progressJson;

        return progressJson.value("isActive", false);

    } catch (...) {
        return false;
    }
}

bool ProgressState::clearProgress() {
    try {
        if (fs::exists(progressFilePath)) {
            fs::remove(progressFilePath);
            LOG_INFO("Progress file cleared");
        }
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to clear progress: " + std::string(e.what()));
        return false;
    }
}

std::string ProgressState::getProgressPath() const {
    return progressFilePath.string();
}

fs::path ProgressState::getDefaultProgressPath() const {
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        return fs::path(appdata) / "fabric-binary-search" / "progress.json";
    }
    return fs::path("progress.json");
#else
    const char* home = std::getenv("HOME");
    if (home) {
        return fs::path(home) / ".config" / "fabric-binary-search" / "progress.json";
    }
    return fs::path("progress.json");
#endif
}