#pragma once

#include <cstddef>

enum class ActiveHighlightMode;
struct Dataset;

class TestControls {
public:
    static void draw(Dataset& dataset,
                     std::size_t& activeSeriesIndex,
                     ActiveHighlightMode& activeHighlightMode,
                     double& xMin,
                     double& xMax,
                     bool& showNativeLegend,
                     bool& showCustomLegend,
                     bool& showValues,
                     bool& showCrosshair);
};
