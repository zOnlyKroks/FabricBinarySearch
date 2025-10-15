#ifndef FABRICBINARYSEARCH_GUIAPP_H
#define FABRICBINARYSEARCH_GUIAPP_H

#ifdef BUILD_GUI

#include "ModManager.h"
#include "BinarySearchEngine.h"
#include "CrashLogParser.h"
#include <string>
#include <memory>

struct GLFWwindow;

class GuiApp {
public:
    GuiApp();
    ~GuiApp();

    bool init();

    void run();

    void cleanup() const;

private:
    GLFWwindow* window = nullptr;
    std::unique_ptr<ModManager> modManager;
    std::unique_ptr<BinarySearchEngine> searchEngine;

    std::string modsPath;
    std::string instancePath;
    bool modsScanned = false;
    bool searchInProgress = false;
    std::string statusMessage;
    std::string crashLogContent;
    std::optional<CrashInfo> lastCrashInfo;

    std::vector<std::string> availableCrashLogs;
    std::vector<std::string> availableGameLogs;
    int selectedLogIndex = -1;
    bool showLogSelector = false;
    bool showInstancePathEditor = false;
    bool showModMetadata = false;
    bool showSettings = false;
    std::string selectedModId;

    bool isScanning = false;
    bool isAnalyzing = false;

    // Panel sizing
    float leftPanelWidth = 0.4f;
    float setupPanelHeight = 0.5f;
    float modListPanelHeight = 0.6f;

    void renderMainWindow();
    void renderSetupPanel();
    void renderModListPanel() const;
    void renderBinarySearchPanel();
    void renderCrashAnalysisPanel();
    void renderStatusBar() const;

    void scanMods();
    void startBinarySearch();
    void reportSuccess();
    void reportFailure();
    void analyzeCrashLog();
    void analyzeCrashLog(const std::string& logPath);
    void refreshLogLists();
    void resetSearch();
    void enableAllMods();
    void launchMinecraft();
    void openFileDialog();
    void renderLogSelectorModal();
    void renderInstancePathModal();
    void renderModMetadataModal();
    void renderSettingsModal();
    bool renderSplitterVertical(const char* id, float* leftWidth, float minLeft, float minRight);
    bool renderSplitterHorizontal(const char* id, float* topHeight, float minTop, float minBottom);
};

#endif // BUILD_GUI

#endif // FABRICBINARYSEARCH_GUIAPP_H