#ifndef FABRICBINARYSEARCH_MODMANAGER_H
#define FABRICBINARYSEARCH_MODMANAGER_H

#include "ModInfo.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>

namespace fs = std::filesystem;

class ModManager {
public:
    explicit ModManager(std::string& modsDirectory);

    bool scanMods();

    [[nodiscard]] const std::vector<ModInfo>& getMods() const { return mods; }

    [[nodiscard]] const ModInfo* getModById(const std::string& modId) const;

    bool disableMods(const std::vector<std::string>& modIds);

    bool enableMods(const std::vector<std::string>& modIds);

    bool disableAllExcept(const std::unordered_set<std::string>& keepEnabled);

    bool enableAllMods();

    [[nodiscard]] std::unordered_set<std::string> getRequiredDependencies(
        const std::unordered_set<std::string>& modIds) const;

    [[nodiscard]] std::vector<std::string> getEnabledModIds() const;

    [[nodiscard]] std::vector<std::string> getDisabledModIds() const;

    void printModList() const;

    void printDependencyGraph() const;

    void setModsDirectory(const std::string& newModsDirectory);

    [[nodiscard]] const std::string& getModsDirectory() const { return modsDir; }

private:
    std::string modsDir;
    std::vector<ModInfo> mods;

    std::unordered_map<std::string, std::string> modIdToPath;


    void collectDependencies(const std::string& modId,
                            std::unordered_set<std::string>& result) const;

    [[nodiscard]] std::string getDisabledPath(const std::string& jarPath) const;

    [[nodiscard]] bool isDisabled(const std::string& jarPath) const;
};

#endif // FABRICBINARYSEARCH_MODMANAGER_H