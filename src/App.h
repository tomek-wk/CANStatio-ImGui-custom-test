#pragma once

#include "Chart.h"
#include "CursorModel.h"
#include "DataGenerator.h"
#include "Dataset.h"
#include "MarkerModel.h"
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
    void handleFullscreenToggle();
    void toggleFullscreen();

    AppOptions options_;
    GLFWwindow* window_ = nullptr;
    bool fullscreen_ = false;
    bool fullscreenToggleKeyDown_ = false;
    int windowedX_ = 100;
    int windowedY_ = 100;
    int windowedWidth_ = 1280;
    int windowedHeight_ = 800;
    Dataset dataset_;
    DataPreset dataPreset_ = DataPreset::Small;
    std::size_t activeSeriesIndex_ = std::numeric_limits<std::size_t>::max();
    ActiveHighlightMode activeHighlightMode_ = ActiveHighlightMode::Halo;
    double xMin_ = 0.0;
    double xMax_ = 0.0;
    bool showCustomLegend_ = true;
    bool showValues_ = true;
    bool showCrosshair_ = true;
    ChartMouseState chartMouseState_;
    ChartLayoutState chartLayoutState_;
    ChartInteractionState chartInteractionState_;
    CursorModel cursorModel_;
    MarkerModel markerModel_;
    TestEngine testEngine_;
};
