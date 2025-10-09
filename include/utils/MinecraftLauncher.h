#ifndef FABRICBINARYSEARCH_MINECRAFTLAUNCHER_H
#define FABRICBINARYSEARCH_MINECRAFTLAUNCHER_H

#include <string>
#include <vector>
#include <unordered_set>
#include <filesystem>

namespace fs = std::filesystem;

class MinecraftLauncher {
public:
    explicit MinecraftLauncher(const std::string& instancePath);

    [[nodiscard]] bool launch() const;

    [[nodiscard]] bool canLaunch() const;

private:
    fs::path instancePath;
    fs::path modsPath;
    fs::path versionsPath;
    fs::path librariesPath;
    fs::path assetsPath;

    [[nodiscard]] std::string findJava() const;

    [[nodiscard]] std::string findVersion() const;

    [[nodiscard]] std::string buildClasspath(const std::string& version) const;

    void collectLibrariesFromVersion(const std::string& version, std::vector<std::string>& classpathEntries, std::unordered_set<std::string>& addedLibraries) const;

    [[nodiscard]] std::vector<std::string> getGameArgs(const std::string& version) const;

    [[nodiscard]] std::vector<std::string> getJvmArgs() const;
};

#endif // FABRICBINARYSEARCH_MINECRAFTLAUNCHER_H