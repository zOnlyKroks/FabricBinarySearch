#ifndef FABRICBINARYSEARCH_BINARYSEARCHENGINE_H
#define FABRICBINARYSEARCH_BINARYSEARCHENGINE_H

#include "ModManager.h"
#include <vector>
#include <unordered_set>
#include <string>

enum class TestResult {
    SUCCESS,
    FAILURE,
    UNKNOWN
};

enum class SearchState {
    NOT_STARTED,
    IN_PROGRESS,
    COMPLETED,
    FAILED
};

class BinarySearchEngine {
public:
    explicit BinarySearchEngine(ModManager& manager);

    void startSearch();

    bool nextIteration();

    void reportResult(TestResult result);

    [[nodiscard]] bool isComplete() const;

    [[nodiscard]] std::vector<std::string> getCulprits() const;

    [[nodiscard]] int getCurrentIteration() const { return iteration; }

    [[nodiscard]] std::string getProgressReport() const;

    [[nodiscard]] const std::vector<std::string>& getSuspects() const { return suspects; }

    void reset();

private:
    ModManager& modManager;
    SearchState state;

    std::vector<std::string> allMods;
    std::vector<std::string> suspects;
    std::vector<std::string> innocent;
    std::vector<std::string> currentlyDisabled;

    int iteration;

    void splitSuspects(std::vector<std::string>& half1, std::vector<std::string>& half2);
};

#endif // FABRICBINARYSEARCH_BINARYSEARCHENGINE_H