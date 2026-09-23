#pragma once

#include <cstddef>

class CursorModel;
struct Dataset;

enum class ActiveHighlightMode {
    Halo,
    Outline,
};

struct ChartMouseState {
    bool plotHovered = false;
    bool withinDataset = false;
    double timeSeconds = 0.0;
};

struct ChartLayoutState {
    float plotX = 0.0F;
    float plotY = 0.0F;
    float plotWidth = 0.0F;
    float plotHeight = 0.0F;
};

class Chart {
public:
    static void draw(Dataset& dataset,
                     std::size_t& activeSeriesIndex,
                     ActiveHighlightMode activeHighlightMode,
                     double& xMin,
                     double& xMax,
                     bool showNativeLegend,
                     bool showCrosshair,
                     ChartMouseState& mouseState,
                     ChartLayoutState& layoutState,
                     CursorModel& cursorModel);
};
