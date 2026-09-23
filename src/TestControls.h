#pragma once

#include <cstddef>

class CursorModel;
class MarkerModel;
enum class ActiveHighlightMode;
enum class DataPreset;
struct Dataset;

class TestControls {
public:
    static bool draw(Dataset& dataset,
                     DataPreset& preset,
                     std::size_t& activeSeriesIndex,
                     ActiveHighlightMode& activeHighlightMode,
                     double& xMin,
                     double& xMax,
                     CursorModel& cursorModel,
                     MarkerModel& markerModel,
                     bool& showCustomLegend,
                     bool& showValues,
                     bool& showCrosshair);
};
