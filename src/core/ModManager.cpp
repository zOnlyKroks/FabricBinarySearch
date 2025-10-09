#include "ModManager.h"
#include "JarReader.h"
#include <iostream>
#include <algorithm>
#include <utility>

ModManager::ModManager(std::string& modsDirectory)
    : modsDir(std::move(modsDirectory)) {
    if (!fs::exists(modsDir)) {
        throw std::runtime_error("Mods directory does not exist: " + modsDir);
    }
}

bool ModManager::scanMods() {
    mods.clear();
    modIdToPath.clear();

    std::cout << "Scanning mods in: " << modsDir << std::endl;

    int jarCount = 0;
    int loadedCount = 0;

    for (const auto& entry : fs::directory_iterator(modsDir)) {
        if (!entry.is_regular_file()) continue;

        std::string filename = entry.path().filename().string();
        std::string jarPath = entry.path().string();
        bool isDisabledMod = false;

        if (filename.ends_with(".disabled")) {
            jarPath = jarPath.substr(0, jarPath.length() - 9);
            isDisabledMod = true;
        }
        else if (!filename.ends_with(".jar")) {
            continue;
        }

        jarCount++;

        std::string actualFilePath = entry.path().string();
        auto jsonContent = JarReader::extractFabricModJson(actualFilePath);
        if (!jsonContent) {
            std::string displayName = isDisabledMod ? filename.substr(0, filename.length() - 9) : filename;
            std::cout << "  [SKIP] " << displayName << " - No fabric.mod.json found" << std::endl;
            continue;
        }

        ModInfo mod;
        mod.jarPath = jarPath;

        if (!mod.parseFromJson(*jsonContent)) {
            std::string displayName = isDisabledMod ? filename.substr(0, filename.length() - 9) : filename;
            std::cerr << "  [ERROR] " << displayName << " - Failed to parse fabric.mod.json" << std::endl;
            continue;
        }

        std::string status = isDisabledMod ? "[DISABLED]" : "[OK]";
        std::string displayName = isDisabledMod ? filename.substr(0, filename.length() - 9) : filename;
        std::cout << "  " << status << " " << mod.id << " v" << mod.version << " (" << displayName << ")" << std::endl;

        mods.push_back(mod);
        modIdToPath[mod.id] = jarPath;
        loadedCount++;
    }

    std::cout << "\nLoaded " << loadedCount << " mods from " << jarCount << " JAR files" << std::endl;

    return loadedCount > 0;
}

const ModInfo* ModManager::getModById(const std::string& modId) const {
    const auto it = std::ranges::find_if(mods,
                                         [&modId](const ModInfo& mod) { return mod.id == modId; });

    return (it != mods.end()) ? &(*it) : nullptr;
}

bool ModManager::disableMods(const std::vector<std::string>& modIds) {
    bool allSuccess = true;

    for (const auto& modId : modIds) {
        auto it = modIdToPath.find(modId);
        if (it == modIdToPath.end()) {
            std::cerr << "Mod not found: " << modId << std::endl;
            allSuccess = false;
            continue;
        }

        const std::string& jarPath = it->second;
        std::string disabledPath = getDisabledPath(jarPath);

        try {
            if (fs::exists(jarPath)) {
                fs::rename(jarPath, disabledPath);
                std::cout << "Disabled: " << modId << std::endl;
            } else if (fs::exists(disabledPath)) {
                std::cout << "Already disabled: " << modId << std::endl;
            } else {
                std::cerr << "File not found: " << jarPath << std::endl;
                allSuccess = false;
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error disabling " << modId << ": " << e.what() << std::endl;
            allSuccess = false;
        }
    }

    return allSuccess;
}

bool ModManager::enableMods(const std::vector<std::string>& modIds) {
    bool allSuccess = true;

    for (const auto& modId : modIds) {
        auto it = modIdToPath.find(modId);
        if (it == modIdToPath.end()) {
            std::cerr << "Mod not found: " << modId << std::endl;
            allSuccess = false;
            continue;
        }

        const std::string& jarPath = it->second;
        std::string disabledPath = getDisabledPath(jarPath);

        try {
            if (fs::exists(disabledPath)) {
                fs::rename(disabledPath, jarPath);
                std::cout << "Enabled: " << modId << std::endl;
            } else if (fs::exists(jarPath)) {
                std::cout << "Already enabled: " << modId << std::endl;
            } else {
                std::cerr << "File not found: " << disabledPath << std::endl;
                allSuccess = false;
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error enabling " << modId << ": " << e.what() << std::endl;
            allSuccess = false;
        }
    }

    return allSuccess;
}

bool ModManager::disableAllExcept(const std::unordered_set<std::string>& keepEnabled) {
    const auto required = getRequiredDependencies(keepEnabled);

    std::vector<std::string> toDisable;
    for (const auto& mod : mods) {
        if (!required.contains(mod.id)) {
            toDisable.push_back(mod.id);
        }
    }

    return disableMods(toDisable);
}

bool ModManager::enableAllMods() {
    std::vector<std::string> allModIds;
    for (const auto& mod : mods) {
        allModIds.push_back(mod.id);
    }
    return enableMods(allModIds);
}

std::unordered_set<std::string> ModManager::getRequiredDependencies(
    const std::unordered_set<std::string>& modIds) const {

    std::unordered_set<std::string> required;

    for (const auto& modId : modIds) {
        required.insert(modId);
        collectDependencies(modId, required);
    }

    return required;
}

std::vector<std::string> ModManager::getEnabledModIds() const {
    std::vector<std::string> enabled;

    for (const auto& mod : mods) {
        if (!isDisabled(mod.jarPath)) {
            enabled.push_back(mod.id);
        }
    }

    return enabled;
}

std::vector<std::string> ModManager::getDisabledModIds() const {
    std::vector<std::string> disabled;

    for (const auto& mod : mods) {
        if (isDisabled(mod.jarPath)) {
            disabled.push_back(mod.id);
        }
    }

    return disabled;
}

void ModManager::printModList() const {
    std::cout << "\n=== Loaded Mods ===" << std::endl;

    for (const auto& mod : mods) {
        std::string status = isDisabled(mod.jarPath) ? "[DISABLED]" : "[ENABLED] ";
        std::cout << status << " " << mod.id << " v" << mod.version;

        if (!mod.depends.empty()) {
            std::cout << " (depends on: ";
            bool first = true;
            for (const auto &depId: mod.depends | std::views::keys) {
                if (!first) std::cout << ", ";
                std::cout << depId;
                first = false;
            }
            std::cout << ")";
        }

        std::cout << std::endl;
    }

    std::cout << "===================\n" << std::endl;
}

void ModManager::printDependencyGraph() const {
    std::cout << "\n=== Dependency Graph ===" << std::endl;

    for (const auto& mod : mods) {
        if (!mod.depends.empty()) {
            std::cout << mod.id << " depends on:" << std::endl;
            for (const auto& [depId, version] : mod.depends) {
                std::cout << "  -> " << depId << " (" << version << ")" << std::endl;
            }
        }
    }

    std::cout << "========================\n" << std::endl;
}

void ModManager::collectDependencies(const std::string& modId,
                                     std::unordered_set<std::string>& result) const {
    const ModInfo* mod = getModById(modId);
    if (!mod) return;

    for (const auto &depId: mod->depends | std::views::keys) {
        if (result.insert(depId).second) {
            collectDependencies(depId, result);
        }
    }
}

std::string ModManager::getDisabledPath(const std::string& jarPath) const {
    return jarPath + ".disabled";
}

bool ModManager::isDisabled(const std::string& jarPath) const {
    return !fs::exists(jarPath) && fs::exists(getDisabledPath(jarPath));
}

void ModManager::setModsDirectory(const std::string& newModsDirectory) {
    if (!fs::exists(newModsDirectory)) {
        throw std::runtime_error("Directory does not exist: " + newModsDirectory);
    }

    if (!fs::is_directory(newModsDirectory)) {
        throw std::runtime_error("Path is not a directory: " + newModsDirectory);
    }

    modsDir = newModsDirectory;
    mods.clear();
    modIdToPath.clear();

    std::cout << "Mods directory changed to: " << modsDir << std::endl;
    std::cout << "Run 'scan' to load mods from the new directory." << std::endl;
}