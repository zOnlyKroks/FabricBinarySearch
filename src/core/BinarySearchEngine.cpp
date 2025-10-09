#include "BinarySearchEngine.h"
#include <iostream>
#include <algorithm>

BinarySearchEngine::BinarySearchEngine(ModManager& manager)
    : modManager(manager), state(SearchState::NOT_STARTED), iteration(0) {}

void BinarySearchEngine::startSearch() {
    std::cout << "\n=== Starting Binary Search ===" << std::endl;

    allMods = modManager.getEnabledModIds();
    suspects = allMods;

    if (suspects.size() < 2) {
        std::cout << "Not enough mods to perform binary search (need at least 2)" << std::endl;
        state = SearchState::FAILED;
        return;
    }

    std::cout << "Starting with " << suspects.size() << " mods" << std::endl;

    iteration = 0;
    innocent.clear();
    currentlyDisabled.clear();
    state = SearchState::IN_PROGRESS;

    nextIteration();
}

bool BinarySearchEngine::nextIteration() {
    if (state != SearchState::IN_PROGRESS) {
        std::cerr << "Search not in progress" << std::endl;
        return false;
    }

    if (suspects.size() == 1) {
        std::cout << "\n=== Found the culprit! ===" << std::endl;
        std::cout << "Problematic mod: " << suspects[0] << std::endl;
        state = SearchState::COMPLETED;
        return false;
    }

    iteration++;
    std::cout << "\n=== Iteration " << iteration << " ===" << std::endl;
    std::cout << "Suspects remaining: " << suspects.size() << std::endl;

    std::vector<std::string> half1, half2;
    splitSuspects(half1, half2);

    std::cout << "\nDisabling " << half1.size() << " mods (keeping " << half2.size() << " enabled):" << std::endl;
    for (const auto& modId : half1) {
        std::cout << "  - " << modId << std::endl;
    }

    std::unordered_set keepEnabled(half2.begin(), half2.end());
    keepEnabled.insert(innocent.begin(), innocent.end());

    modManager.disableAllExcept(keepEnabled);
    currentlyDisabled = half1;

    std::cout << "\n*** Please test Minecraft now ***" << std::endl;
    std::cout << "After testing, report the result:" << std::endl;
    std::cout << "  - If problem is GONE -> type 'success'" << std::endl;
    std::cout << "  - If problem PERSISTS -> type 'failure'" << std::endl;

    return true;
}

void BinarySearchEngine::reportResult(TestResult result) {
    if (state != SearchState::IN_PROGRESS) {
        std::cerr << "No search in progress" << std::endl;
        return;
    }

    if (result == TestResult::SUCCESS) {
        std::cout << "\nProblem resolved! Culprit is in disabled set." << std::endl;
        suspects = currentlyDisabled;

        for (const auto& modId : modManager.getEnabledModIds()) {
            if (std::ranges::find(innocent, modId) == innocent.end()) {
                innocent.push_back(modId);
            }
        }

    } else if (result == TestResult::FAILURE) {
        std::cout << "\nProblem persists. Culprit is in enabled set." << std::endl;

        std::vector<std::string> newSuspects;
        for (const auto& modId : modManager.getEnabledModIds()) {
            if (std::ranges::find(innocent, modId) == innocent.end()) {
                newSuspects.push_back(modId);
            }
        }
        suspects = newSuspects;
        innocent.insert(innocent.end(), currentlyDisabled.begin(), currentlyDisabled.end());
    }

    if (suspects.size() == 1) {
        std::cout << "\n=== Search Complete ===" << std::endl;
        std::cout << "Problematic mod identified: " << suspects[0] << std::endl;
        state = SearchState::COMPLETED;
        return;
    }

    if (suspects.empty()) {
        std::cout << "\n=== Search Failed ===" << std::endl;
        std::cout << "Could not identify a single problematic mod." << std::endl;
        std::cout << "Possible reasons:" << std::endl;
        std::cout << "  - Multiple mods causing the issue together" << std::endl;
        std::cout << "  - Problem is not mod-related" << std::endl;
        state = SearchState::FAILED;
        return;
    }

    nextIteration();
}

bool BinarySearchEngine::isComplete() const {
    return state == SearchState::COMPLETED || state == SearchState::FAILED;
}

std::vector<std::string> BinarySearchEngine::getCulprits() const {
    if (state == SearchState::COMPLETED && !suspects.empty()) {
        return suspects;
    }
    return {};
}

std::string BinarySearchEngine::getProgressReport() const {
    std::string report;
    report += "Iteration: " + std::to_string(iteration) + "\n";
    report += "Suspects: " + std::to_string(suspects.size()) + "\n";
    report += "Innocent: " + std::to_string(innocent.size()) + "\n";

    if (state == SearchState::COMPLETED) {
        report += "Status: COMPLETED\n";
    } else if (state == SearchState::IN_PROGRESS) {
        report += "Status: IN PROGRESS\n";
    } else {
        report += "Status: NOT STARTED\n";
    }

    return report;
}

void BinarySearchEngine::reset() {
    suspects.clear();
    innocent.clear();
    currentlyDisabled.clear();
    allMods.clear();
    iteration = 0;
    state = SearchState::NOT_STARTED;
    modManager.enableAllMods();
}

void BinarySearchEngine::splitSuspects(std::vector<std::string>& half1, std::vector<std::string>& half2) {
    size_t midpoint = suspects.size() / 2;
    half1.assign(suspects.begin(), suspects.begin() + midpoint);
    half2.assign(suspects.begin() + midpoint, suspects.end());
}