#ifdef BUILD_GUI

#include "GuiApp.h"
#include "MinecraftLauncher.h"
#include "Config.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

GuiApp::GuiApp() {
    const std::string home = std::getenv("HOME") ? std::getenv("HOME") : "";

#ifdef __APPLE__
    instancePath = home + "/Library/Application Support/minecraft";
    modsPath = instancePath + "/mods";
#elif defined(_WIN32)
    std::string appdata = std::getenv("APPDATA") ? std::getenv("APPDATA") : "";
    instancePath = appdata + "\\.minecraft";
    modsPath = instancePath + "\\mods";
#else
    instancePath = home + "/.minecraft";
    modsPath = instancePath + "/mods";
#endif
}

GuiApp::~GuiApp() {
    cleanup();
}

bool GuiApp::init() {
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // GL 3.3 + GLSL 330
    const auto glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create resizable window
    window = glfwCreateWindow(1280, 720, "Fabric Binary Search Tool", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Enable persistent layout saving
    io.IniFilename = "fabricbinarysearch.ini";

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    return true;
}

void GuiApp::run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderMainWindow();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

void GuiApp::cleanup() const {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

void GuiApp::renderMainWindow() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
                                              ImGuiWindowFlags_NoMove |
                                              ImGuiWindowFlags_NoResize |
                                              ImGuiWindowFlags_NoSavedSettings |
                                              ImGuiWindowFlags_MenuBar;

    ImGui::Begin("MainWindow", nullptr, window_flags);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Settings...")) {
                showSettings = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset All", nullptr, false, modsScanned)) {
                resetSearch();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(window, true);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                ImGui::OpenPopup("About");
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    // About popup modal
    if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Fabric Binary Search Tool - C++ Edition");
        ImGui::Text("Find problematic Minecraft mods using binary search");
        ImGui::Separator();
        ImGui::Text("Built with Dear ImGui, GLFW, and C++20");
        ImGui::Spacing();
        ImGui::Text("Features:");
        ImGui::BulletText("Resizable window and panels");
        ImGui::BulletText("Resizable table columns");
        ImGui::BulletText("Persistent layout configuration");
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Fabric Binary Search Tool");
    ImGui::PopFont();
    ImGui::Separator();

    // Use child regions with borders for resizable panels
    ImGui::BeginChild("ContentArea", ImVec2(0, -30), false);

    const float availWidth = ImGui::GetContentRegionAvail().x;
    const float availHeight = ImGui::GetContentRegionAvail().y;

    // Left panel - Setup and Binary Search
    ImGui::BeginChild("LeftPanel", ImVec2(availWidth * leftPanelWidth, availHeight), true);

    const float leftPanelHeight = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("SetupSection", ImVec2(0, leftPanelHeight * setupPanelHeight), false);
    renderSetupPanel();
    ImGui::EndChild();

    // Horizontal splitter between Setup and Binary Search
    renderSplitterHorizontal("LeftSplitter", &setupPanelHeight, 0.2f, 0.2f);

    ImGui::BeginChild("BinarySearchSection", ImVec2(0, 0), false);
    renderBinarySearchPanel();
    ImGui::EndChild();

    ImGui::EndChild();

    // Vertical splitter between Left and Right panels
    ImGui::SameLine();
    renderSplitterVertical("MainSplitter", &leftPanelWidth, 0.2f, 0.2f);
    ImGui::SameLine();

    // Right panel - Mod List and Crash Analysis
    ImGui::BeginChild("RightPanel", ImVec2(0, availHeight), true);

    const float rightPanelHeight = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("ModListSection", ImVec2(0, rightPanelHeight * modListPanelHeight), false);
    renderModListPanel();
    ImGui::EndChild();

    // Horizontal splitter between Mod List and Crash Analysis
    renderSplitterHorizontal("RightSplitter", &modListPanelHeight, 0.2f, 0.2f);

    ImGui::BeginChild("CrashAnalysisSection", ImVec2(0, 0), false);
    renderCrashAnalysisPanel();
    ImGui::EndChild();

    ImGui::EndChild();

    ImGui::EndChild();

    renderStatusBar();

    renderModMetadataModal();
    renderSettingsModal();

    ImGui::End();
}

void GuiApp::renderSetupPanel() {
    if (ImGui::CollapsingHeader("Setup", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Minecraft Instance:");
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("The .minecraft folder (or MultiMC/Prism instance folder)");
        }

        char instanceBuf[512];
        strncpy(instanceBuf, instancePath.c_str(), sizeof(instanceBuf) - 1);
        instanceBuf[sizeof(instanceBuf) - 1] = '\0';

        if (ImGui::InputText("##instancePath", instanceBuf, sizeof(instanceBuf))) {
            instancePath = instanceBuf;
            modsPath = (fs::path(instancePath) / "mods").string();
        }

        ImGui::SameLine();
        if (ImGui::Button("Change...")) {
            showInstancePathEditor = true;
        }

        ImGui::Spacing();
        ImGui::Text("Mods Directory:");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", modsPath.c_str());

        ImGui::Spacing();
        if (isScanning) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Scanning mods...");
            ImGui::ProgressBar(-1.0f * ImGui::GetTime(), ImVec2(-1, 0));
        } else {
            if (ImGui::Button("Scan Mods", ImVec2(-1, 0))) {
                scanMods();
            }
        }

        if (modsScanned && modManager) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                             "Loaded %zu mods", modManager->getMods().size());

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Enable All Mods", ImVec2(-1, 0))) {
                enableAllMods();
            }

            if (ImGui::Button("Launch Minecraft", ImVec2(-1, 0))) {
                launchMinecraft();
            }
        }
    }

    renderInstancePathModal();
}

void GuiApp::renderModListPanel() const {
    if (ImGui::CollapsingHeader("Mod List", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (!modsScanned || !modManager) {
            ImGui::TextDisabled("Scan mods first");
            return;
        }

        ImGui::BeginChild("ModListChild", ImVec2(0, 0), false);

        const auto modsCopy = modManager->getMods();
        auto enabledModIds = modManager->getEnabledModIds();

        // Use resizable table
        if (ImGui::BeginTable("ModsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Mod ID", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthFixed, 60);
            ImGui::TableHeadersRow();

            for (const auto& mod : modsCopy) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                const bool isEnabled = std::ranges::find(enabledModIds,
                                                   mod.id) != enabledModIds.end();

                if (isEnabled) {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "ENABLED");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "DISABLED");
                }

                ImGui::TableNextColumn();
                ImGui::Text("%s", mod.id.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", mod.version.c_str());

                ImGui::TableNextColumn();
                std::string buttonLabel = "Info##" + mod.id;
                if (ImGui::SmallButton(buttonLabel.c_str())) {
                    const_cast<GuiApp*>(this)->selectedModId = mod.id;
                    const_cast<GuiApp*>(this)->showModMetadata = true;
                }
            }

            ImGui::EndTable();
        }

        ImGui::EndChild();
    }
}

void GuiApp::renderBinarySearchPanel() {
    if (ImGui::CollapsingHeader("Binary Search", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (!modsScanned || !modManager) {
            ImGui::TextDisabled("Scan mods first");
            return;
        }

        if (!searchInProgress) {
            if (ImGui::Button("Start Binary Search", ImVec2(-1, 40))) {
                startBinarySearch();
            }
        } else {
            if (!searchEngine) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Search engine not initialized");
                searchInProgress = false;
                return;
            }

            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
                             "Iteration %d", searchEngine->getCurrentIteration());

            const auto suspects = searchEngine->getSuspects();
            ImGui::Text("Suspects remaining: %zu", suspects.size());

            ImGui::Separator();
            ImGui::TextWrapped("Test Minecraft now and report the result:");

            if (ImGui::Button("Problem GONE (Success)", ImVec2(-1, 40))) {
                reportSuccess();
            }

            if (ImGui::Button("Problem PERSISTS (Failure)", ImVec2(-1, 40))) {
                reportFailure();
            }

            ImGui::Separator();

            if (ImGui::Button("Reset Search")) {
                resetSearch();
            }
        }
    }
}

void GuiApp::renderCrashAnalysisPanel() {
    if (ImGui::CollapsingHeader("Crash Analysis")) {
        ImGui::BeginGroup();

        if (isAnalyzing) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Analyzing log...");
            ImGui::ProgressBar(-1.0f * ImGui::GetTime(), ImVec2(-1, 0));
        } else {
            if (ImGui::Button("Select Log to Analyze...", ImVec2(-1, 0))) {
                refreshLogLists();
                showLogSelector = true;
            }

            if (ImGui::Button("Analyze Latest Crash Log", ImVec2(-1, 0))) {
                analyzeCrashLog();
            }
        }

        ImGui::EndGroup();

        if (lastCrashInfo) {
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::BeginChild("CrashInfoChild", ImVec2(0, 300), true);

            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Crash Information");
            ImGui::Separator();

            ImGui::Text("Crash Type:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "%s",
                             lastCrashInfo->crashType.empty() ? "Unknown" : lastCrashInfo->crashType.c_str());

            ImGui::Spacing();
            ImGui::Text("Error Message:");
            ImGui::TextWrapped("%s", lastCrashInfo->errorMessage.c_str());

            ImGui::Spacing();
            ImGui::Separator();

            if (!lastCrashInfo->suspectedMods.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Suspected Mods:");

                for (const auto& mod : lastCrashInfo->suspectedMods) {
                    if (mod == lastCrashInfo->primarySuspect) {
                        ImGui::BulletText("");
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                                         "%s (PRIMARY SUSPECT)", mod.c_str());
                    } else {
                        ImGui::BulletText("%s", mod.c_str());
                    }
                }
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No specific mods identified");
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Columns(2, nullptr, false);
            ImGui::Checkbox("Mixin Error", &lastCrashInfo->isMixinError);
            ImGui::NextColumn();
            ImGui::Checkbox("Mod Loading Error", &lastCrashInfo->isModLoadingError);
            ImGui::Columns(1);

            if (!lastCrashInfo->stackTrace.empty()) {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::TreeNode("Stack Trace")) {
                    ImGui::BeginChild("StackTraceChild", ImVec2(0, 150), true);
                    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

                    for (const auto& line : lastCrashInfo->stackTrace) {
                        ImGui::TextUnformatted(line.c_str());
                    }

                    ImGui::PopFont();
                    ImGui::EndChild();
                    ImGui::TreePop();
                }
            }

            ImGui::EndChild();
        }
    }

    renderLogSelectorModal();
}

void GuiApp::renderStatusBar() const {
    ImGui::Separator();
    if (!statusMessage.empty()) {
        ImGui::TextWrapped("%s", statusMessage.c_str());
    } else {
        ImGui::TextDisabled("Ready");
    }
}

void GuiApp::scanMods() {
    isScanning = true;
    try {
        modManager = std::make_unique<ModManager>(modsPath);
        if (modManager->scanMods()) {
            modsScanned = true;
            searchEngine = std::make_unique<BinarySearchEngine>(*modManager);
            statusMessage = "Mods scanned successfully! Loaded " + std::to_string(modManager->getMods().size()) + " mods.";
        } else {
            statusMessage = "Failed to scan mods. No mods found.";
        }
    } catch (const std::exception& e) {
        statusMessage = "Error: " + std::string(e.what());
        modsScanned = false;
    }
    isScanning = false;
}

void GuiApp::startBinarySearch() {
    if (!searchEngine) {
        statusMessage = "Error: Search engine not initialized!";
        return;
    }

    try {
        searchEngine->startSearch();
        searchInProgress = true;
        statusMessage = "Binary search started! Test Minecraft and report results.";
    } catch (const std::exception& e) {
        statusMessage = "Error starting search: " + std::string(e.what());
        searchInProgress = false;
    }
}

void GuiApp::reportSuccess() {
    if (!searchEngine) return;

    searchEngine->reportResult(TestResult::SUCCESS);

    if (searchEngine->isComplete()) {
        if (const auto culprits = searchEngine->getCulprits(); !culprits.empty()) {
            statusMessage = "Found culprit: " + culprits[0];
            searchInProgress = false;
        } else {
            statusMessage = "Search failed - no single culprit found.";
            searchInProgress = false;
        }
    } else {
        statusMessage = "Problem gone! Continuing search with disabled mods...";
    }
}

void GuiApp::reportFailure() {
    if (!searchEngine) return;

    searchEngine->reportResult(TestResult::FAILURE);

    if (searchEngine->isComplete()) {
        if (const auto culprits = searchEngine->getCulprits(); !culprits.empty()) {
            statusMessage = "Found culprit: " + culprits[0];
            searchInProgress = false;
        } else {
            statusMessage = "Search failed - no single culprit found.";
            searchInProgress = false;
        }
    } else {
        statusMessage = "Problem persists! Continuing search with enabled mods...";
    }
}

void GuiApp::analyzeCrashLog() {
    if (!modsScanned || modsPath.empty()) {
        statusMessage = "Please scan mods first.";
        return;
    }

    const std::string crashDir = fs::path(modsPath).parent_path().string() + "/crash-reports";

    if (const auto latestLog = CrashLogParser::findLatestCrashLog(crashDir)) {
        lastCrashInfo = CrashLogParser::parseCrashLog(*latestLog);
        if (lastCrashInfo) {
            statusMessage = "Crash log analyzed: " + fs::path(*latestLog).filename().string();
        } else {
            statusMessage = "Failed to parse crash log.";
        }
    } else {
        statusMessage = "No crash logs found in: " + crashDir;
    }
}

void GuiApp::resetSearch() {
    if (searchEngine) {
        searchEngine->reset();
    }
    searchInProgress = false;
    statusMessage = "Search reset. All mods re-enabled.";
}

void GuiApp::analyzeCrashLog(const std::string& logPath) {
    isAnalyzing = true;
    lastCrashInfo = CrashLogParser::parseCrashLog(logPath);
    isAnalyzing = false;

    if (lastCrashInfo) {
        statusMessage = "Analyzed: " + fs::path(logPath).filename().string();
    } else {
        statusMessage = "Failed to parse log file.";
    }
}

void GuiApp::enableAllMods() {
    if (!modManager) {
        statusMessage = "Error: Mod manager not initialized!";
        return;
    }

    try {
        modManager->enableAllMods();
        statusMessage = "All mods have been enabled!";
    } catch (const std::exception& e) {
        statusMessage = "Error enabling mods: " + std::string(e.what());
    }
}

void GuiApp::launchMinecraft() {
    if (!modsScanned || instancePath.empty()) {
        statusMessage = "Please scan mods first.";
        return;
    }

    try {
        const MinecraftLauncher launcher(instancePath);

        if (!launcher.canLaunch()) {
            statusMessage = "Error: Cannot launch Minecraft from this location.\n"
                          "This feature requires a standard Minecraft installation with versions/ and libraries/ directories.\n"
                          "For third-party launchers, please launch manually from the launcher.";

            std::string command;
#ifdef __APPLE__
            command = "open \"" + instancePath + "\"";
#elif defined(_WIN32)
            command = "explorer \"" + instancePath + "\"";
#else
            command = "xdg-open \"" + instancePath + "\"";
#endif
            system(command.c_str());
            return;
        }

        statusMessage = "Launching Minecraft... Please wait for the game to start.";

        if (launcher.launch()) {
            statusMessage = "Minecraft launched successfully! Test for the issue, then report success or failure.";
        } else {
            statusMessage = "Failed to launch Minecraft. Please check the console output for details.\n"
                          "Try launching manually from your launcher instead.";
        }
    } catch (const std::exception& e) {
        statusMessage = "Launch error: " + std::string(e.what()) +
                       "\nPlease launch Minecraft manually from your launcher.";
    }
}

void GuiApp::refreshLogLists() {
    availableCrashLogs.clear();
    availableGameLogs.clear();

    if (!instancePath.empty()) {
        const fs::path instance(instancePath);
        const std::string crashDir = (instance / "crash-reports").string();
        const std::string logsDir = (instance / "logs").string();

        availableCrashLogs = CrashLogParser::listCrashLogs(crashDir);
        availableGameLogs = CrashLogParser::listGameLogs(logsDir);
    }
}

void GuiApp::openFileDialog() {
    //TODO: Implement file dialog for changing instance path
    statusMessage = "Use the instance path text field to enter a path manually.";
}

void GuiApp::renderLogSelectorModal() {
    if (showLogSelector) {
        ImGui::OpenPopup("Select Log File");
        showLogSelector = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Select Log File", nullptr, ImGuiWindowFlags_NoResize)) {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Select a crash or game log to analyze:");
        ImGui::Separator();

        ImGui::BeginChild("LogList", ImVec2(0, -35), true);

        if (!availableCrashLogs.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Crash Reports:");
            ImGui::Separator();

            for (size_t i = 0; i < availableCrashLogs.size(); ++i) {
                fs::path logPath(availableCrashLogs[i]);
                std::string filename = logPath.filename().string();

                auto writeTime = fs::last_write_time(logPath);
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    writeTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
                std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
                char timeBuf[100];
                std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&tt));

                bool isSelected = (selectedLogIndex == static_cast<int>(i));
                std::string label = filename + " (" + std::string(timeBuf) + ")";

                if (ImGui::Selectable(label.c_str(), isSelected)) {
                    selectedLogIndex = static_cast<int>(i);
                }

                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::Spacing();
            ImGui::Spacing();
        }

        if (!availableGameLogs.empty()) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Game Logs:");
            ImGui::Separator();

            for (size_t i = 0; i < availableGameLogs.size(); ++i) {
                fs::path logPath(availableGameLogs[i]);
                std::string filename = logPath.filename().string();

                auto writeTime = fs::last_write_time(logPath);
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    writeTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
                std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
                char timeBuf[100];
                std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&tt));

                int gameLogIndex = static_cast<int>(availableCrashLogs.size() + i);
                bool isSelected = (selectedLogIndex == gameLogIndex);
                std::string label = filename + " (" + std::string(timeBuf) + ")";

                if (ImGui::Selectable(label.c_str(), isSelected)) {
                    selectedLogIndex = gameLogIndex;
                }

                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
        }

        if (availableCrashLogs.empty() && availableGameLogs.empty()) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No log files found.");
            ImGui::TextWrapped("Make sure you've scanned mods first and that crash-reports or logs directories exist.");
        }

        ImGui::EndChild();

        ImGui::Separator();

        if (ImGui::Button("Analyze Selected", ImVec2(120, 0))) {
            if (selectedLogIndex >= 0) {
                if (selectedLogIndex < static_cast<int>(availableCrashLogs.size())) {
                    analyzeCrashLog(availableCrashLogs[selectedLogIndex]);
                } else {
                    int gameLogIdx = selectedLogIndex - static_cast<int>(availableCrashLogs.size());
                    analyzeCrashLog(availableGameLogs[gameLogIdx]);
                }
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void GuiApp::renderInstancePathModal() {
    if (showInstancePathEditor) {
        ImGui::OpenPopup("Change Instance Path");
        showInstancePathEditor = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 250), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Change Instance Path", nullptr, ImGuiWindowFlags_NoResize)) {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Change Minecraft Instance Path");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextWrapped("Enter the path to your Minecraft instance folder. This should be the folder containing the 'mods', 'crash-reports', and 'logs' directories.");
        ImGui::Spacing();

        ImGui::Text("Common locations:");
        ImGui::BulletText("Default: ~/.minecraft or %%APPDATA%%\\.minecraft");
        ImGui::BulletText("MultiMC: ~/MultiMC/instances/[instance]/.minecraft");
        ImGui::BulletText("Prism Launcher: ~/.local/share/PrismLauncher/instances/[instance]/.minecraft");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        static char pathBuf[512];
        if (ImGui::IsWindowAppearing()) {
            strncpy(pathBuf, instancePath.c_str(), sizeof(pathBuf) - 1);
            pathBuf[sizeof(pathBuf) - 1] = '\0';
        }

        ImGui::Text("Instance Path:");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##newInstancePath", pathBuf, sizeof(pathBuf));

        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            std::string newPath = pathBuf;

            if (!newPath.empty() && newPath[0] == '~') {
                if (const char* home = std::getenv("HOME")) {
                    newPath = std::string(home) + newPath.substr(1);
                }
            }

            if (fs::exists(newPath)) {
                instancePath = newPath;
                modsPath = (fs::path(instancePath) / "mods").string();
                statusMessage = "Instance path updated to: " + instancePath;
                ImGui::CloseCurrentPopup();
            } else {
                statusMessage = "Error: Path does not exist: " + newPath;
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

bool GuiApp::renderSplitterVertical(const char* id, float* leftWidth, float minLeft, float minRight) {
    ImGui::PushID(id);

    const ImGuiStyle& style = ImGui::GetStyle();
    const float splitterWidth = 8.0f;

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));

    ImGui::Button("##vsplitter", ImVec2(splitterWidth, -1));

    ImGui::PopStyleColor(3);

    bool changed = false;
    if (ImGui::IsItemActive()) {
        const float availWidth = ImGui::GetContentRegionAvail().x + splitterWidth;
        const float delta = ImGui::GetIO().MouseDelta.x / availWidth;

        float newWidth = *leftWidth + delta;
        newWidth = std::max(minLeft, std::min(1.0f - minRight, newWidth));

        if (newWidth != *leftWidth) {
            *leftWidth = newWidth;
            changed = true;
        }
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }

    ImGui::PopID();
    return changed;
}

bool GuiApp::renderSplitterHorizontal(const char* id, float* topHeight, float minTop, float minBottom) {
    ImGui::PushID(id);

    const float splitterHeight = 8.0f;

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));

    ImGui::Button("##hsplitter", ImVec2(-1, splitterHeight));

    ImGui::PopStyleColor(3);

    bool changed = false;
    if (ImGui::IsItemActive()) {
        const float availHeight = ImGui::GetContentRegionAvail().y + splitterHeight;
        const float delta = ImGui::GetIO().MouseDelta.y / availHeight;

        float newHeight = *topHeight + delta;
        newHeight = std::max(minTop, std::min(1.0f - minBottom, newHeight));

        if (newHeight != *topHeight) {
            *topHeight = newHeight;
            changed = true;
        }
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }

    ImGui::PopID();
    return changed;
}

void GuiApp::renderModMetadataModal() {
    if (showModMetadata) {
        ImGui::OpenPopup("Mod Metadata");
        showModMetadata = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(900, 700), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Mod Metadata", nullptr, ImGuiWindowFlags_None)) {
        if (!modManager || selectedModId.empty()) {
            ImGui::Text("No mod selected");
            if (ImGui::Button("Close", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
            return;
        }

        // Find the selected mod
        const auto& mods = modManager->getMods();
        auto modIt = std::ranges::find_if(mods, [this](const ModInfo& m) {
            return m.id == selectedModId;
        });

        if (modIt == mods.end()) {
            ImGui::Text("Mod not found");
            if (ImGui::Button("Close", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
            return;
        }

        const ModInfo& mod = *modIt;

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", mod.name.c_str());
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "ID: %s | Version: %s",
                          mod.id.c_str(), mod.version.c_str());
        ImGui::Separator();

        if (ImGui::BeginTabBar("ModMetadataTabs")) {
            // Overview tab
            if (ImGui::BeginTabItem("Overview")) {
                ImGui::BeginChild("OverviewContent", ImVec2(0, -35), true);

                if (!mod.description.empty()) {
                    ImGui::TextWrapped("%s", mod.description.c_str());
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                }

                if (!mod.authors.empty()) {
                    ImGui::Text("Authors:");
                    for (const auto& author : mod.authors) {
                        ImGui::BulletText("%s", author.c_str());
                    }
                    ImGui::Spacing();
                }

                ImGui::Text("Environment: %s", mod.environment.c_str());
                ImGui::Text("JAR Path: %s", mod.jarPath.c_str());
                ImGui::Spacing();

                if (!mod.depends.empty()) {
                    ImGui::Separator();
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Dependencies:");
                    for (const auto& [depId, version] : mod.depends) {
                        ImGui::BulletText("%s: %s", depId.c_str(), version.c_str());
                    }
                    ImGui::Spacing();
                }

                if (!mod.suggests.empty()) {
                    ImGui::Separator();
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Suggestions:");
                    for (const auto& [sugId, version] : mod.suggests) {
                        ImGui::BulletText("%s: %s", sugId.c_str(), version.c_str());
                    }
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // Links tab
            if (ImGui::BeginTabItem("Links")) {
                ImGui::BeginChild("LinksContent", ImVec2(0, -35), true);

                bool hasLinks = false;

                if (!mod.homepage.empty()) {
                    hasLinks = true;
                    ImGui::Text("Homepage:");
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", mod.homepage.c_str());
                    if (ImGui::IsItemClicked()) {
                        std::string command;
#ifdef __APPLE__
                        command = "open \"" + mod.homepage + "\"";
#elif defined(_WIN32)
                        command = "start \"\" \"" + mod.homepage + "\"";
#else
                        command = "xdg-open \"" + mod.homepage + "\"";
#endif
                        system(command.c_str());
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Click to open in browser");
                    }
                    ImGui::Spacing();
                }

                if (!mod.sources.empty()) {
                    hasLinks = true;
                    ImGui::Text("Source Code:");
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", mod.sources.c_str());
                    if (ImGui::IsItemClicked()) {
                        std::string command;
#ifdef __APPLE__
                        command = "open \"" + mod.sources + "\"";
#elif defined(_WIN32)
                        command = "start \"\" \"" + mod.sources + "\"";
#else
                        command = "xdg-open \"" + mod.sources + "\"";
#endif
                        system(command.c_str());
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Click to open in browser");
                    }
                    ImGui::Spacing();
                }

                if (!mod.issues.empty()) {
                    hasLinks = true;
                    ImGui::Text("Issue Tracker:");
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", mod.issues.c_str());
                    if (ImGui::IsItemClicked()) {
                        std::string command;
#ifdef __APPLE__
                        command = "open \"" + mod.issues + "\"";
#elif defined(_WIN32)
                        command = "start \"\" \"" + mod.issues + "\"";
#else
                        command = "xdg-open \"" + mod.issues + "\"";
#endif
                        system(command.c_str());
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Click to open in browser");
                    }
                    ImGui::Spacing();
                }

                // Display all other contact fields
                for (const auto& [key, value] : mod.contact) {
                    if (key != "homepage" && key != "sources" && key != "issues") {
                        hasLinks = true;
                        ImGui::Text("%s:", key.c_str());
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", value.c_str());
                        if (value.find("http://") == 0 || value.find("https://") == 0) {
                            if (ImGui::IsItemClicked()) {
                                std::string command;
#ifdef __APPLE__
                                command = "open \"" + value + "\"";
#elif defined(_WIN32)
                                command = "start \"\" \"" + value + "\"";
#else
                                command = "xdg-open \"" + value + "\"";
#endif
                                system(command.c_str());
                            }
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip("Click to open in browser");
                            }
                        }
                        ImGui::Spacing();
                    }
                }

                if (!hasLinks) {
                    ImGui::TextDisabled("No contact information or links available");
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // Raw JSON tab
            if (ImGui::BeginTabItem("fabric.mod.json")) {
                ImGui::BeginChild("RawJsonContent", ImVec2(0, -35), true);

                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
                ImGui::TextUnformatted(mod.rawJson.c_str());
                ImGui::PopFont();

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Separator();

        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void GuiApp::renderSettingsModal() {
    if (showSettings) {
        ImGui::OpenPopup("Settings");
        showSettings = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(700, 400), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Settings", nullptr, ImGuiWindowFlags_NoResize)) {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Application Settings");
        ImGui::Separator();
        ImGui::Spacing();

        static char launchCommandBuf[1024];

        if (ImGui::BeginTabBar("SettingsTabs")) {
            if (ImGui::BeginTabItem("Launcher")) {
                ImGui::BeginChild("LauncherSettings", ImVec2(0, -40), true);

                ImGui::TextWrapped("Custom Launch Command:");
                ImGui::Spacing();
                ImGui::TextWrapped("Leave empty to use the built-in launcher. Enter a custom command to override it.");
                ImGui::TextWrapped("This is useful for third-party launchers or custom launch configurations.");
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::IsWindowAppearing()) {
                    std::string currentCommand = Config::getInstance().getLaunchCommand();
                    strncpy(launchCommandBuf, currentCommand.c_str(), sizeof(launchCommandBuf) - 1);
                    launchCommandBuf[sizeof(launchCommandBuf) - 1] = '\0';
                }

                ImGui::Text("Launch Command:");
                ImGui::SetNextItemWidth(-1);
                ImGui::InputTextMultiline("##launchCommand", launchCommandBuf, sizeof(launchCommandBuf), ImVec2(-1, 100));

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Example: /path/to/your/launcher --instance YourInstance");
                ImGui::Spacing();

                if (ImGui::Button("Clear", ImVec2(120, 0))) {
                    launchCommandBuf[0] = '\0';
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Separator();

        if (ImGui::Button("Save", ImVec2(120, 0))) {
            Config::getInstance().setLaunchCommand(launchCommandBuf);
            Config::getInstance().save();
            statusMessage = "Settings saved successfully!";
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

#endif // BUILD_GUI