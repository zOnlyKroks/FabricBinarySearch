#ifndef FABRICBINARYSEARCH_MODINFO_H
#define FABRICBINARYSEARCH_MODINFO_H

#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ModInfo {
public:
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::string jarPath;

    std::unordered_map<std::string, std::string> depends;
    std::unordered_map<std::string, std::string> suggests;

    std::vector<std::string> authors;
    std::string environment;

    bool parseFromJson(const std::string& jsonContent);

    [[nodiscard]] bool dependsOn(const std::string& modId) const;

    [[nodiscard]] std::vector<std::string> getDependencies() const;

    void print() const;

private:
    static void parseDependencies(const json& j, const std::string& key,
                                  std::unordered_map<std::string, std::string>& target);
};

#endif // FABRICBINARYSEARCH_MODINFO_H