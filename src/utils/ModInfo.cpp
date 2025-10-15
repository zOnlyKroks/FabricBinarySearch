#include "ModInfo.h"
#include <iostream>
#include <regex>

static std::string sanitizeJson(const std::string& jsonContent) {
    std::string sanitized = jsonContent;
    std::regex controlChars(R"(("(?:[^"\\]|\\.)*"))");

    std::string result;
    auto begin = std::sregex_iterator(sanitized.begin(), sanitized.end(), controlChars);
    auto end = std::sregex_iterator();

    size_t lastPos = 0;
    for (auto it = begin; it != end; ++it) {
        result.append(sanitized, lastPos, it->position() - lastPos);

        std::string matched = it->str();
        std::string cleaned;

        for (size_t i = 0; i < matched.length(); ++i) {
            if (char c = matched[i]; c < 0x20 && c != '\t' && i > 0 && matched[i-1] != '\\') {
                switch (c) {
                    case '\n': cleaned += "\\n"; break;
                    case '\r': cleaned += "\\r"; break;
                    case '\t': cleaned += "\\t"; break;
                    default: break;
                }
            } else {
                cleaned += c;
            }
        }

        result.append(cleaned);
        lastPos = it->position() + it->length();
    }

    result.append(sanitized, lastPos, sanitized.length() - lastPos);
    return result.empty() ? sanitized : result;
}

bool ModInfo::parseFromJson(const std::string& jsonContent) {
    try {
        std::string sanitized = sanitizeJson(jsonContent);
        json j = json::parse(sanitized);

        // Store raw JSON for full metadata display
        rawJson = j.dump(2);

        if (!j.contains("id") || !j.contains("version")) {
            std::cerr << "Missing required fields (id or version) in fabric.mod.json" << std::endl;
            return false;
        }

        id = j["id"].get<std::string>();

        if (j["version"].is_string()) {
            version = j["version"].get<std::string>();
        } else if (j["version"].is_object()) {
            if (j["version"].contains("en_us")) {
                version = j["version"]["en_us"].get<std::string>();
            } else {
                version = j["version"].begin().value().get<std::string>();
            }
        } else {
            version = j["version"].dump();
        }

        if (j.contains("name")) {
            if (j["name"].is_string()) {
                name = j["name"].get<std::string>();
            } else if (j["name"].is_object()) {
                if (j["name"].contains("en_us")) {
                    name = j["name"]["en_us"].get<std::string>();
                } else {
                    name = j["name"].begin().value().get<std::string>();
                }
            } else {
                name = id;
            }
        } else {
            name = id;
        }

        if (j.contains("description")) {
            if (j["description"].is_string()) {
                description = j["description"].get<std::string>();
            } else if (j["description"].is_object()) {
                if (j["description"].contains("en_us")) {
                    description = j["description"]["en_us"].get<std::string>();
                } else {
                    description = j["description"].begin().value().get<std::string>();
                }
            }
        }

        if (j.contains("environment")) {
            environment = j["environment"].get<std::string>();
        } else {
            environment = "*";
        }

        if (j.contains("authors")) {
            if (j["authors"].is_array()) {
                for (const auto& author : j["authors"]) {
                    if (author.is_string()) {
                        authors.push_back(author.get<std::string>());
                    } else if (author.is_object() && author.contains("name")) {
                        authors.push_back(author["name"].get<std::string>());
                    }
                }
            }
        }

        parseDependencies(j, "depends", depends);
        parseDependencies(j, "suggests", suggests);

        // Parse contact/links metadata
        if (j.contains("contact")) {
            const auto& contactObj = j["contact"];
            if (contactObj.is_object()) {
                if (contactObj.contains("homepage")) {
                    homepage = contactObj["homepage"].get<std::string>();
                }
                if (contactObj.contains("sources")) {
                    sources = contactObj["sources"].get<std::string>();
                }
                if (contactObj.contains("issues")) {
                    issues = contactObj["issues"].get<std::string>();
                }
                // Store all contact fields
                for (auto it = contactObj.begin(); it != contactObj.end(); ++it) {
                    if (it.value().is_string()) {
                        contact[it.key()] = it.value().get<std::string>();
                    }
                }
            }
        }

        return true;

    } catch (const json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        return false;
    }
}

void ModInfo::parseDependencies(const json& j, const std::string& key,
                                std::unordered_map<std::string, std::string>& target) {
    if (!j.contains(key)) return;

    const auto& deps = j[key];
    if (!deps.is_object()) return;

    for (auto it = deps.begin(); it != deps.end(); ++it) {
        const std::string& modId = it.key();
        std::string versionReq;

        if (it.value().is_string()) {
            versionReq = it.value().get<std::string>();
        } else if (it.value().is_array()) {
            versionReq = it.value().dump();
        }

        target[modId] = versionReq;
    }
}

bool ModInfo::dependsOn(const std::string& modId) const {
    return depends.contains(modId);
}

std::vector<std::string> ModInfo::getDependencies() const {
    std::vector<std::string> deps;
    for (const auto &modId: depends | std::views::keys) {
        deps.push_back(modId);
    }
    return deps;
}

void ModInfo::print() const {
    std::cout << "\n=== Mod Info ===" << std::endl;
    std::cout << "ID: " << id << std::endl;
    std::cout << "Name: " << name << std::endl;
    std::cout << "Version: " << version << std::endl;
    std::cout << "Path: " << jarPath << std::endl;

    if (!depends.empty()) {
        std::cout << "Dependencies:" << std::endl;
        for (const auto& [modId, version] : depends) {
            std::cout << "  - " << modId << " (" << version << ")" << std::endl;
        }
    }

    std::cout << "================\n" << std::endl;
}