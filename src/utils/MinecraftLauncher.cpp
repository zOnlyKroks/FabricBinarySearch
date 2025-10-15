#include "MinecraftLauncher.h"
#include "Config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

MinecraftLauncher::MinecraftLauncher(const std::string& instancePath)
    : instancePath(instancePath) {

    modsPath = fs::path(instancePath) / "mods";
    versionsPath = fs::path(instancePath) / "versions";
    librariesPath = fs::path(instancePath) / "libraries";
    assetsPath = fs::path(instancePath) / "assets";
}

bool MinecraftLauncher::canLaunch() const {
    return fs::exists(versionsPath) && fs::exists(librariesPath);
}

std::string MinecraftLauncher::findJava() const {
    const char* javaHome = std::getenv("JAVA_HOME");
    if (javaHome) {
        fs::path javaPath = fs::path(javaHome) / "bin" / "java";
#ifdef _WIN32
        javaPath += ".exe";
#endif
        if (fs::exists(javaPath)) {
            return javaPath.string();
        }
    }

#ifdef __APPLE__
    if (FILE* pipe = popen("/usr/libexec/java_home 2>/dev/null", "r")) {
        char buffer[512];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe)) {
            result += buffer;
        }
        pclose(pipe);

        if (!result.empty()) {
            result.erase(result.find_last_not_of(" \n\r\t") + 1);
            if (const fs::path javaPath = fs::path(result) / "bin" / "java"; fs::exists(javaPath)) {
                return javaPath.string();
            }
        }
    }
#endif

#ifdef _WIN32
    const char* programFiles[] = {
        std::getenv("ProgramFiles"),
        std::getenv("ProgramFiles(x86)"),
        "C:\\Program Files",
        "C:\\Program Files (x86)"
    };

    std::vector<std::string> javaSearchPaths = {
        "Java\\jdk*\\bin\\java.exe",
        "Java\\jre*\\bin\\java.exe",
        "Eclipse Adoptium\\jdk*\\bin\\java.exe",
        "Zulu\\zulu-*\\bin\\java.exe"
    };

    for (const char* pf : programFiles) {
        if (!pf) continue;
        for (const auto& searchPath : javaSearchPaths) {
            fs::path basePath = fs::path(pf);
            try {
                for (const auto& entry : fs::directory_iterator(basePath)) {
                    if (entry.is_directory()) {
                        fs::path javaExe = entry.path() / "bin" / "java.exe";
                        if (fs::exists(javaExe)) {
                            return javaExe.string();
                        }
                    }
                }
            } catch (...) {}
        }
    }
#endif

#ifdef __linux__
    FILE* pipe = popen("which java 2>/dev/null", "r");
    if (pipe) {
        char buffer[512];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe)) {
            result += buffer;
        }
        pclose(pipe);

        if (!result.empty()) {
            result.erase(result.find_last_not_of(" \n\r\t") + 1);
            if (fs::exists(result)) {
                return result;
            }
        }
    }

    std::vector<std::string> linuxPaths = {
        "/usr/lib/jvm/default-java/bin/java",
        "/usr/lib/jvm/java-17-openjdk/bin/java",
        "/usr/lib/jvm/java-21-openjdk/bin/java",
        "/usr/lib/jvm/java-17-openjdk-amd64/bin/java",
        "/usr/lib/jvm/java-21-openjdk-amd64/bin/java"
    };

    for (const auto& path : linuxPaths) {
        if (fs::exists(path)) {
            return path;
        }
    }
#endif

    const std::vector<std::string> fallbackPaths = {
#ifdef __APPLE__
        "/usr/bin/java",
        "/Library/Java/JavaVirtualMachines/jdk-17.jdk/Contents/Home/bin/java",
        "/Library/Java/JavaVirtualMachines/jdk-21.jdk/Contents/Home/bin/java"
#elif defined(_WIN32)
        "java.exe"
#else
        "/usr/bin/java"
#endif
    };

    for (const auto& path : fallbackPaths) {
        if (fs::exists(path)) {
            return path;
        }
    }

    return "java";
}

std::string MinecraftLauncher::findVersion() const {
    if (!fs::exists(versionsPath)) {
        return "";
    }

    // Look for Fabric versions first, then any version
    std::string fabricVersion;
    std::string anyVersion;

    for (const auto& entry : fs::directory_iterator(versionsPath)) {
        if (entry.is_directory()) {
            std::string versionName = entry.path().filename().string();

            if (versionName.find("fabric") != std::string::npos) {
                fabricVersion = versionName;
            }

            if (anyVersion.empty()) {
                anyVersion = versionName;
            }
        }
    }

    return fabricVersion.empty() ? anyVersion : fabricVersion;
}

void MinecraftLauncher::collectLibrariesFromVersion(const std::string& version, std::vector<std::string>& classpathEntries, std::unordered_set<std::string>& addedLibraries) const {
    fs::path versionJson = versionsPath / version / (version + ".json");
    if (!fs::exists(versionJson)) {
        return;
    }

    std::ifstream file(versionJson);
    json j = json::parse(file);

    if (j.contains("inheritsFrom")) {
        std::string parentVersion = j["inheritsFrom"];
        std::cout << "  Loading parent version: " << parentVersion << std::endl;

        collectLibrariesFromVersion(parentVersion, classpathEntries, addedLibraries);
    }

    if (fs::path versionJar = versionsPath / version / (version + ".jar"); fs::exists(versionJar)) {
        classpathEntries.push_back(versionJar.string());
    }

    if (j.contains("libraries")) {
        for (const auto& lib : j["libraries"]) {
            if (lib.contains("name")) {
                std::string libName = lib["name"];

                size_t colon1 = libName.find(':');
                size_t colon2 = libName.find(':', colon1 + 1);

                if (colon1 != std::string::npos && colon2 != std::string::npos) {
                    std::string group = libName.substr(0, colon1);
                    std::string artifact = libName.substr(colon1 + 1, colon2 - colon1 - 1);
                    std::string version = libName.substr(colon2 + 1);

                    std::string libraryKey = group + ":" + artifact;

                    if (addedLibraries.contains(libraryKey)) {
                        std::cout << "  Skipping duplicate library: " << libName << " (already have " << libraryKey << ")" << std::endl;
                        continue;
                    }

                    addedLibraries.insert(libraryKey);

                    std::ranges::replace(group, '.', '/');

                    if (fs::path libPath = librariesPath / group / artifact / version / (artifact + "-" + version + ".jar"); fs::exists(libPath)) {
                        classpathEntries.push_back(libPath.string());
                    }
                }
            }
        }
    }
}

std::string MinecraftLauncher::buildClasspath(const std::string& version) const {
    std::vector<std::string> classpathEntries;
    std::unordered_set<std::string> addedLibraries;

    collectLibrariesFromVersion(version, classpathEntries, addedLibraries);

#ifdef _WIN32
    char separator = ';';
#else
    char separator = ':';
#endif

    std::string classpath;
    for (size_t i = 0; i < classpathEntries.size(); ++i) {
        classpath += classpathEntries[i];
        if (i < classpathEntries.size() - 1) {
            classpath += separator;
        }
    }

    return classpath;
}

std::vector<std::string> MinecraftLauncher::getJvmArgs() const {
    const std::string nativesPath = (instancePath / "natives").string();

    return {
        "-Xmx2G",
        "-Xms512M",
        "-Djava.library.path=" + nativesPath
    };
}

std::vector<std::string> MinecraftLauncher::getGameArgs(const std::string& version) const {
    std::vector<std::string> args;

    args.emplace_back("--gameDir");
    args.push_back(instancePath.string());

    args.emplace_back("--assetsDir");
    args.push_back(assetsPath.string());

    args.emplace_back("--version");
    args.push_back(version);

    args.emplace_back("--username");
    args.emplace_back("Player");

    args.emplace_back("--uuid");
    args.emplace_back("00000000-0000-0000-0000-000000000000");

    return args;
}

bool MinecraftLauncher::launch() const {
    if (!canLaunch()) {
        std::cerr << "Cannot launch: Missing required Minecraft files" << std::endl;
        std::cerr << "Make sure you have:" << std::endl;
        std::cerr << "  - versions directory: " << versionsPath << std::endl;
        std::cerr << "  - libraries directory: " << librariesPath << std::endl;
        return false;
    }

    std::string customCommand = Config::getInstance().getLaunchCommand();
    if (!customCommand.empty()) {
        std::cout << "\nExecuting custom launch command..." << std::endl;
        std::cout << "Command: " << customCommand << std::endl;

#ifdef _WIN32
        std::string windowsCmd = "start /B cmd /C \"" + customCommand + "\"";
        int result = system(windowsCmd.c_str());
#else
        std::string unixCmd = customCommand + " &";
        int result = system(unixCmd.c_str());
#endif

        if (result == 0) {
            std::cout << "\nMinecraft launched successfully!" << std::endl;
            std::cout << "Please test for the issue, then type 'success' or 'failure'" << std::endl;
            return true;
        } else {
            std::cerr << "Failed to launch Minecraft (exit code: " << result << ")" << std::endl;
            return false;
        }
    }

    std::string javaPath = findJava();
    std::string version = findVersion();

    if (version.empty()) {
        std::cerr << "No Minecraft version found in: " << versionsPath << std::endl;
        return false;
    }

    std::cout << "Found version: " << version << std::endl;
    std::cout << "Java: " << javaPath << std::endl;

    std::stringstream cmd;
    cmd << "\"" << javaPath << "\"";

    for (const auto& arg : getJvmArgs()) {
        if (arg.find("-Djava.library.path=") == 0) {
            std::string path = arg.substr(20);
            cmd << " \"-Djava.library.path=" << path << "\"";
        } else {
            cmd << " " << arg;
        }
    }

    std::string classpath = buildClasspath(version);
    cmd << " -cp \"" << classpath << "\"";

    cmd << " net.fabricmc.loader.impl.launch.knot.KnotClient";

    for (const auto& arg : getGameArgs(version)) {
        cmd << " \"" << arg << "\"";
    }

    std::cout << "\nLaunching Minecraft..." << std::endl;
    std::cout << "Command: " << cmd.str() << std::endl;

#ifdef _WIN32
    std::string windowsCmd = "start /B cmd /C \"" + cmd.str() + "\"";
    int result = system(windowsCmd.c_str());
#else
    std::string unixCmd = cmd.str() + " &";
    int result = system(unixCmd.c_str());
#endif

    if (result == 0) {
        std::cout << "\nMinecraft launched successfully!" << std::endl;
        std::cout << "Please test for the issue, then type 'success' or 'failure'" << std::endl;
        return true;
    } else {
        std::cerr << "Failed to launch Minecraft (exit code: " << result << ")" << std::endl;
        return false;
    }
}
