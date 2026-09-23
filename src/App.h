#pragma once

#include "Chart.h"
#include "CursorModel.h"
#include "Dataset.h"
#include "TestEngine.h"

#include <cstddef>
#include <limits>
#include <string>

struct GLFWwindow;

struct AppOptions {
    bool runTests = false;
    bool showTestUi = false;
    std::string testFilter;
    std::string testResultsPath;
};

class App {
public:
    explicit App(AppOptions options = {});
    int run();

private:
    bool initialize();
    void shutdown();
    void frame();

    AppOptions options_;
    GLFWwindow* window_ = nullptr;
    Dataset dataset_;
    std::size_t activeSeriesIndex_ = std::numeric_limits<std::size_t>::max();
    ActiveHighlightMode activeHighlightMode_ = ActiveHighlightMode::Halo;
    double xMin_ = 0.0;
    double xMax_ = 0.0;
    bool showCustomLegend_ = true;
    bool showValues_ = true;
    bool showCrosshair_ = true;
    ChartMouseState chartMouseState_;
    ChartLayoutState chartLayoutState_;
    CursorModel cursorModel_;
    TestEngine testEngine_;
};
