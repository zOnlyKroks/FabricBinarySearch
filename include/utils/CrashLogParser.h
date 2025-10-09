#ifndef FABRICBINARYSEARCH_CRASHLOGPARSER_H
#define FABRICBINARYSEARCH_CRASHLOGPARSER_H

#include <string>
#include <vector>
#include <optional>

struct CrashInfo {
    std::string crashType;
    std::vector<std::string> suspectedMods;
    std::string primarySuspect;
    std::vector<std::string> stackTrace;
    bool isMixinError = false;
    bool isModLoadingError = false;
    std::string errorMessage;
};

class CrashLogParser {
public:
    static std::optional<CrashInfo> parseCrashLog(const std::string& logPath);

    static std::optional<CrashInfo> parseCrashLogContent(const std::string& content);

    static std::optional<std::string> findLatestCrashLog(const std::string& crashReportsDir);

    static std::vector<std::string> listCrashLogs(const std::string& crashDir);

    static std::vector<std::string> listGameLogs(const std::string& logsDir);

private:
    static std::optional<std::string> extractModIdFromStackTrace(const std::string& line);

    static bool isMixinCrash(const std::string& content);

    static bool isModLoadingCrash(const std::string& content);

    static std::string extractErrorMessage(const std::string& content);
};

#endif // FABRICBINARYSEARCH_CRASHLOGPARSER_H