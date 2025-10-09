#include <iostream>
#include <string>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include "ModManager.h"
#include "BinarySearchEngine.h"
#include "CrashLogParser.h"
#include "MinecraftLauncher.h"

#ifdef BUILD_GUI
#include "GuiApp.h"
#endif

namespace fs = std::filesystem;

void printBanner() {
    std::cout << R"(
╔═══════════════════════════════════════════════════════╗
║   Fabric Binary Search Tool - C++ Edition            ║
║   Find problematic Minecraft mods automatically      ║
╚═══════════════════════════════════════════════════════╝
)" << std::endl;
}

void printHelp() {
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  scan                  - Scan mods directory" << std::endl;
    std::cout << "  list                  - List all mods" << std::endl;
    std::cout << "  deps                  - Show dependency graph" << std::endl;
    std::cout << "  logs                  - List all crash logs and game logs" << std::endl;
    std::cout << "  analyze [log_file]    - Analyze a crash/game log" << std::endl;
    std::cout << "  start                 - Start binary search" << std::endl;
    std::cout << "  success               - Report test succeeded (problem gone)" << std::endl;
    std::cout << "  failure               - Report test failed (problem persists)" << std::endl;
    std::cout << "  stop                  - Stop binary search and show results" << std::endl;
    std::cout << "  reset                 - Reset and enable all mods" << std::endl;
    std::cout << "  setpath <path>        - Set custom Minecraft instance path" << std::endl;
    std::cout << "  launch                - Launch Minecraft" << std::endl;
    std::cout << "  help                  - Show this help" << std::endl;
    std::cout << "  quit                  - Exit program" << std::endl;
#ifdef BUILD_GUI
    std::cout << "\nTo use GUI mode, run with: --gui or -g" << std::endl;
#endif
}

std::string getMinecraftModsPath() {
    // Try to auto-detect .minecraft/mods directory
    std::string home = std::getenv("HOME") ? std::getenv("HOME") : "";
    std::vector<std::string> possiblePaths;

#ifdef __APPLE__
    possiblePaths = {
        home + "/Library/Application Support/minecraft/mods",
        home + "/.minecraft/mods"
    };
#elif defined(_WIN32)
    std::string appdata = std::getenv("APPDATA") ? std::getenv("APPDATA") : "";
    possiblePaths = {
        appdata + "\\.minecraft\\mods",
        appdata + "\\minecraft\\mods"
    };
#else
    possiblePaths = {
        home + "/.minecraft/mods",
        home + "/minecraft/mods"
    };
#endif

    for (const auto& path : possiblePaths) {
        if (fs::exists(path)) {
            std::cout << "Auto-detected mods directory: " << path << std::endl;
            return path;
        }
    }

    std::cout << "Could not auto-detect mods directory." << std::endl;
    std::cout << "Enter path to Minecraft mods directory: ";
    std::string path;
    std::getline(std::cin, path);
    return path;
}

int main(int argc, char** argv) {
    // Check for --gui flag
    bool useGui = false;

#ifdef BUILD_GUI
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--gui" || arg == "-g") {
            useGui = true;
            break;
        }
    }

    if (useGui) {
        GuiApp app;
        if (!app.init()) {
            std::cerr << "Failed to initialize GUI" << std::endl;
            return 1;
        }
        app.run();
        return 0;
    }
#endif

    // CLI Mode
    printBanner();

    std::string modsPath = getMinecraftModsPath();

    try {
        ModManager modManager(modsPath);
        BinarySearchEngine searchEngine(modManager);

        std::cout << "\nType 'help' for available commands\n" << std::endl;

        bool running = true;
        while (running) {
            std::cout << "> ";
            std::string command;
            std::getline(std::cin, command);

            if (command.empty()) continue;

            // Parse command
            size_t spacePos = command.find(' ');
            std::string cmd = command.substr(0, spacePos);
            std::string args = spacePos != std::string::npos ? command.substr(spacePos + 1) : "";

            if (cmd == "scan") {
                modManager.scanMods();

            } else if (cmd == "list") {
                modManager.printModList();

            } else if (cmd == "deps") {
                modManager.printDependencyGraph();

            } else if (cmd == "logs") {
                fs::path instancePath = fs::path(modManager.getModsDirectory()).parent_path();
                std::string crashDir = (instancePath / "crash-reports").string();
                std::string logsDir = (instancePath / "logs").string();

                // List crash logs
                auto crashLogs = CrashLogParser::listCrashLogs(crashDir);
                if (!crashLogs.empty()) {
                    std::cout << "\n=== Crash Logs ===" << std::endl;
                    for (size_t i = 0; i < crashLogs.size(); ++i) {
                        fs::path logPath(crashLogs[i]);
                        auto writeTime = fs::last_write_time(logPath);
                        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                            writeTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                        );
                        std::time_t tt = std::chrono::system_clock::to_time_t(sctp);

                        std::cout << "  [" << (i + 1) << "] " << logPath.filename().string();
                        std::cout << " (" << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S") << ")" << std::endl;
                    }
                    std::cout << "\nTo analyze: analyze <number> or analyze <path>" << std::endl;
                } else {
                    std::cout << "\nNo crash logs found in: " << crashDir << std::endl;
                }

                // List game logs
                auto gameLogs = CrashLogParser::listGameLogs(logsDir);
                if (!gameLogs.empty()) {
                    std::cout << "\n=== Game Logs ===" << std::endl;
                    for (size_t i = 0; i < gameLogs.size() && i < 10; ++i) {  // Show max 10
                        fs::path logPath(gameLogs[i]);
                        auto writeTime = fs::last_write_time(logPath);
                        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                            writeTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                        );
                        std::time_t tt = std::chrono::system_clock::to_time_t(sctp);

                        std::cout << "  [" << (crashLogs.size() + i + 1) << "] " << logPath.filename().string();
                        std::cout << " (" << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S") << ")" << std::endl;
                    }
                    std::cout << "\nTo analyze: analyze <number> or analyze <path>" << std::endl;
                } else {
                    std::cout << "\nNo game logs found in: " << logsDir << std::endl;
                }

            } else if (cmd == "analyze") {
                std::string logPath;

                if (args.empty()) {
                    // Try to find latest crash log
                    std::string crashDir = fs::path(modsPath).parent_path().string() + "/crash-reports";
                    auto latestLog = CrashLogParser::findLatestCrashLog(crashDir);

                    if (latestLog) {
                        logPath = *latestLog;
                    } else {
                        std::cout << "No crash logs found. Use 'logs' to list available logs." << std::endl;
                        continue;
                    }
                } else {
                    // Check if args is a number (selecting from logs list)
                    try {
                        int logNum = std::stoi(args);
                        fs::path instancePath = fs::path(modManager.getModsDirectory()).parent_path();

                        // Get all logs
                        auto crashLogs = CrashLogParser::listCrashLogs((instancePath / "crash-reports").string());
                        auto gameLogs = CrashLogParser::listGameLogs((instancePath / "logs").string());

                        if (logNum > 0 && logNum <= static_cast<int>(crashLogs.size())) {
                            logPath = crashLogs[logNum - 1];
                        } else if (logNum > static_cast<int>(crashLogs.size()) &&
                                   logNum <= static_cast<int>(crashLogs.size() + gameLogs.size())) {
                            logPath = gameLogs[logNum - crashLogs.size() - 1];
                        } else {
                            std::cerr << "Invalid log number. Use 'logs' to see available logs." << std::endl;
                            continue;
                        }
                    } catch (const std::invalid_argument&) {
                        // Not a number, treat as path
                        logPath = args;
                    }
                }

                // Analyze the log
                std::cout << "Analyzing: " << logPath << std::endl;
                auto crashInfo = CrashLogParser::parseCrashLog(logPath);

                if (crashInfo) {
                    std::cout << "\n=== Log Analysis ===" << std::endl;
                    std::cout << "Error: " << crashInfo->errorMessage << std::endl;
                    std::cout << "Type: " << crashInfo->crashType << std::endl;

                    if (!crashInfo->suspectedMods.empty()) {
                        std::cout << "\nSuspected mods:" << std::endl;
                        for (const auto& mod : crashInfo->suspectedMods) {
                            std::cout << "  - " << mod;
                            if (mod == crashInfo->primarySuspect) {
                                std::cout << " (PRIMARY SUSPECT)";
                            }
                            std::cout << std::endl;
                        }
                    }

                    std::cout << "\nMixin error: " << (crashInfo->isMixinError ? "Yes" : "No") << std::endl;
                    std::cout << "Mod loading error: " << (crashInfo->isModLoadingError ? "Yes" : "No") << std::endl;
                } else {
                    std::cerr << "Failed to analyze log file." << std::endl;
                }

            } else if (cmd == "start") {
                searchEngine.startSearch();

            } else if (cmd == "success") {
                searchEngine.reportResult(TestResult::SUCCESS);

            } else if (cmd == "failure") {
                searchEngine.reportResult(TestResult::FAILURE);

            } else if (cmd == "stop") {
                searchEngine.reset();
                std::cout << "\n=== Binary Search Stopped ===" << std::endl;
                std::cout << "All mods have been re-enabled." << std::endl;
                std::cout << "\nTo start a new search, use the 'start' command." << std::endl;

            } else if (cmd == "reset") {
                searchEngine.reset();
                std::cout << "All mods re-enabled" << std::endl;

            } else if (cmd == "setpath") {
                if (args.empty()) {
                    std::cout << "Usage: setpath <path_to_minecraft_instance>" << std::endl;
                    std::cout << "Example: setpath ~/MultiMC/instances/MyInstance/.minecraft" << std::endl;
                    std::cout << "\nCurrent path: " << modManager.getModsDirectory() << std::endl;
                } else {
                    try {
                        // Expand ~ to home directory
                        std::string expandedPath = args;
                        if (!expandedPath.empty() && expandedPath[0] == '~') {
                            const char* home = std::getenv("HOME");
                            if (home) {
                                expandedPath = std::string(home) + expandedPath.substr(1);
                            }
                        }

                        // Support both instance root and mods directory
                        fs::path inputPath(expandedPath);
                        fs::path modsPath;

                        if (fs::exists(inputPath / "mods")) {
                            // User provided instance root (e.g., .minecraft)
                            modsPath = inputPath / "mods";
                        } else if (fs::exists(inputPath) && inputPath.filename() == "mods") {
                            // User provided mods directory directly
                            modsPath = inputPath;
                        } else {
                            std::cerr << "Error: Could not find 'mods' directory in: " << expandedPath << std::endl;
                            std::cout << "Please provide either:" << std::endl;
                            std::cout << "  - Path to Minecraft instance (containing 'mods' folder)" << std::endl;
                            std::cout << "  - Path to 'mods' directory directly" << std::endl;
                            continue;
                        }

                        modManager.setModsDirectory(modsPath.string());
                    } catch (const std::exception& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                    }
                }

            } else if (cmd == "launch") {
                try {
                    fs::path instancePath = fs::path(modManager.getModsDirectory()).parent_path();

                    if (MinecraftLauncher launcher(instancePath.string()); !launcher.canLaunch()) {
                        std::cerr << "Error: Cannot launch Minecraft from this location." << std::endl;
                        std::cerr << "This feature requires a standard Minecraft installation with:" << std::endl;
                        std::cerr << "  - versions/ directory" << std::endl;
                        std::cerr << "  - libraries/ directory" << std::endl;
                        std::cerr << "\nFor third-party launchers, please launch manually from the launcher." << std::endl;
                        std::cerr << "Once in-game, test your issue, then return here and type 'success' or 'failure'." << std::endl;
                    } else {
                        launcher.launch();
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Launch error: " << e.what() << std::endl;
                    std::cerr << "\nPlease launch Minecraft manually from your launcher." << std::endl;
                    std::cerr << "Once in-game, test your issue, then return here and type 'success' or 'failure'." << std::endl;
                }

            } else if (cmd == "help") {
                printHelp();

            } else if (cmd == "quit" || cmd == "exit") {
                running = false;

            } else {
                std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
            }
        }

        std::cout << "\nGoodbye!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}