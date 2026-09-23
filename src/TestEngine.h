#pragma once

#include <cstddef>
#include <string>

class CursorModel;
struct ChartLayoutState;
struct ChartMouseState;
struct Dataset;
struct ImGuiTestEngine;
enum class ActiveHighlightMode;

struct GuiTestAccess {
    Dataset* dataset = nullptr;
    std::size_t* activeSeriesIndex = nullptr;
    ActiveHighlightMode* activeHighlightMode = nullptr;
    double* xMin = nullptr;
    double* xMax = nullptr;
    bool* showCustomLegend = nullptr;
    bool* showValues = nullptr;
    bool* showCrosshair = nullptr;
    ChartMouseState* chartMouseState = nullptr;
    ChartLayoutState* chartLayoutState = nullptr;
    CursorModel* cursorModel = nullptr;
};

class TestEngine {
public:
    bool initialize(bool runTests,
                    bool showUi,
                    const std::string& filter,
                    const std::string& resultsPath,
                    const GuiTestAccess& access);
    void stop();
    void destroy();
    void drawUi();
    void preSwap();
    void postSwap();

    bool testsComplete() const;
    bool shouldExitAfterTests() const;
    int resultCode() const;
    void printResultSummary() const;

private:
    ImGuiTestEngine* engine_ = nullptr;
    bool started_ = false;
    bool runTests_ = false;
    bool showUi_ = false;
    bool testsQueued_ = false;
    std::string resultsPath_;
};
