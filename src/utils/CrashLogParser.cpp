#include "CrashLogParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <regex>
#include <algorithm>
#include <zlib.h>

namespace fs = std::filesystem;

std::optional<CrashInfo> CrashLogParser::parseCrashLog(const std::string& logPath) {
    std::string content;

    if (logPath.ends_with(".gz")) {
        gzFile gzf = gzopen(logPath.c_str(), "rb");
        if (!gzf) {
            std::cerr << "Could not open compressed log: " << logPath << std::endl;
            return std::nullopt;
        }

        char buffer[8192];
        int bytesRead;
        std::stringstream ss;

        while ((bytesRead = gzread(gzf, buffer, sizeof(buffer))) > 0) {
            ss.write(buffer, bytesRead);
        }

        gzclose(gzf);
        content = ss.str();

        if (bytesRead < 0) {
            std::cerr << "Error reading compressed log: " << logPath << std::endl;
            return std::nullopt;
        }
    } else {
        std::ifstream file(logPath);
        if (!file.is_open()) {
            std::cerr << "Could not open log: " << logPath << std::endl;
            return std::nullopt;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        content = buffer.str();
    }

    return parseCrashLogContent(content);
}

std::optional<CrashInfo> CrashLogParser::parseCrashLogContent(const std::string& content) {
    CrashInfo info;

    info.isMixinError = isMixinCrash(content);
    info.isModLoadingError = isModLoadingCrash(content);
    info.errorMessage = extractErrorMessage(content);

    std::istringstream stream(content);
    std::string line;
    bool inStackTrace = false;

    while (std::getline(stream, line)) {
        if (line.find("at ") == 0 || line.find("\tat ") != std::string::npos) {
            inStackTrace = true;
            info.stackTrace.push_back(line);

            if (auto modId = extractModIdFromStackTrace(line)) {
                if (std::ranges::find(info.suspectedMods, *modId)
                    == info.suspectedMods.end()) {
                    info.suspectedMods.push_back(*modId);

                    if (info.primarySuspect.empty()) {
                        info.primarySuspect = *modId;
                    }
                }
            }
        } else if (inStackTrace && line.empty()) {
            inStackTrace = false;
        }

        if (line.find("incompatible") != std::string::npos ||
            line.find("requires") != std::string::npos ||
            line.find("depends") != std::string::npos) {

            std::regex modPattern(R"(\(([a-z0-9_-]+)\))");
            std::smatch match;
            auto searchStart(line.cbegin());

            while (std::regex_search(searchStart, line.cend(), match, modPattern)) {
                if (std::string modId = match[1].str(); std::ranges::find(info.suspectedMods, modId)
                                                        == info.suspectedMods.end()) {
                    info.suspectedMods.push_back(modId);

                    if (info.primarySuspect.empty()) {
                        info.primarySuspect = modId;
                    }
                }
                searchStart = match.suffix().first;
            }

            if (info.errorMessage == "Unknown error") {
                info.errorMessage = line;
            }
        }

        if (line.find("Exception") != std::string::npos ||
            line.find("Error") != std::string::npos) {

            std::regex exceptionRegex(R"(([A-Za-z.]+(?:Exception|Error)))");
            if (std::smatch match; std::regex_search(line, match, exceptionRegex)) {
                if (info.crashType.empty()) {
                    info.crashType = match[1].str();
                }
            }
        }
    }

    if (info.suspectedMods.empty()) {
        std::cout << "No specific mod identified in log" << std::endl;
        return std::nullopt;
    }

    return info;
}

std::optional<std::string> CrashLogParser::findLatestCrashLog(const std::string& crashReportsDir) {
    if (!fs::exists(crashReportsDir) || !fs::is_directory(crashReportsDir)) {
        std::cerr << "Crash reports directory not found: " << crashReportsDir << std::endl;
        return std::nullopt;
    }

    std::optional<fs::path> latestFile;
    fs::file_time_type latestTime;

    for (const auto& entry : fs::directory_iterator(crashReportsDir)) {
        if (entry.is_regular_file()) {
            if (std::string filename = entry.path().filename().string(); filename.find("crash-") == 0 && filename.ends_with(".txt")) {
                if (auto modTime = entry.last_write_time(); !latestFile || modTime > latestTime) {
                    latestFile = entry.path();
                    latestTime = modTime;
                }
            }
        }
    }

    if (latestFile) {
        return latestFile->string();
    }

    return std::nullopt;
}

std::optional<std::string> CrashLogParser::extractModIdFromStackTrace(const std::string& line) {
    const std::regex mixinPattern(R"(([a-z0-9_-]+)\$[a-zA-Z0-9_]+)");
    std::smatch match;

    if (std::regex_search(line, match, mixinPattern)) {
        return match[1].str();
    }

    if (const std::regex packagePattern(R"((?:net|com|org)\.(?:fabricmc|modded|minecraft|[a-z0-9_]+)\.([a-z0-9_-]+))"); std::regex_search(line, match, packagePattern)) {
        if (std::string potentialModId = match[1].str(); potentialModId != "minecraft" &&
                                                         potentialModId != "loader" &&
                                                         potentialModId != "api" &&
                                                         potentialModId != "impl" &&
                                                         potentialModId != "mixin") {
            return potentialModId;
        }
    }

    return std::nullopt;
}

bool CrashLogParser::isMixinCrash(const std::string& content) {
    return content.find("Mixin") != std::string::npos ||
           content.find("mixin") != std::string::npos ||
           content.find("@Inject") != std::string::npos;
}

bool CrashLogParser::isModLoadingCrash(const std::string& content) {
    return content.find("ModResolutionException") != std::string::npos ||
           content.find("Failed to load mod") != std::string::npos ||
           content.find("Missing dependency") != std::string::npos;
}

std::string CrashLogParser::extractErrorMessage(const std::string& content) {
    const size_t startPos = content.find("---- Minecraft Crash Report ----");
    if (startPos == std::string::npos) {
        return "Unknown error";
    }

    const size_t lineStart = content.find('\n', startPos) + 1;

    if (const size_t lineEnd = content.find('\n', lineStart); lineStart != std::string::npos && lineEnd != std::string::npos) {
        return content.substr(lineStart, lineEnd - lineStart);
    }

    return "Unknown error";
}

std::vector<std::string> CrashLogParser::listCrashLogs(const std::string& crashDir) {
    std::vector<fs::path> crashLogs;

    if (!fs::exists(crashDir) || !fs::is_directory(crashDir)) {
        return {};
    }

    for (const auto& entry : fs::directory_iterator(crashDir)) {
        if (entry.is_regular_file()) {
            if (std::string filename = entry.path().filename().string(); filename.find("crash-") != std::string::npos && filename.ends_with(".txt")) {
                crashLogs.push_back(entry.path());
            }
        }
    }

    std::ranges::sort(crashLogs, [](const fs::path& a, const fs::path& b) {
        return fs::last_write_time(a) > fs::last_write_time(b);
    });

    std::vector<std::string> result;
    result.reserve(crashLogs.size());

    for (const auto& path : crashLogs) {
        result.push_back(path.string());
    }

    return result;
}

std::vector<std::string> CrashLogParser::listGameLogs(const std::string& logsDir) {
    std::vector<fs::path> gameLogs;

    if (!fs::exists(logsDir) || !fs::is_directory(logsDir)) {
        return {};
    }

    for (const auto& entry : fs::directory_iterator(logsDir)) {
        if (entry.is_regular_file()) {
            if (std::string filename = entry.path().filename().string(); filename.ends_with(".log") || filename.ends_with(".log.gz")) {
                gameLogs.push_back(entry.path());
            }
        }
    }

    std::ranges::sort(gameLogs, [](const fs::path& a, const fs::path& b) {
        return fs::last_write_time(a) > fs::last_write_time(b);
    });

    std::vector<std::string> result;
    result.reserve(gameLogs.size());

    for (const auto& path : gameLogs) {
        result.push_back(path.string());
    }
    return result;
}