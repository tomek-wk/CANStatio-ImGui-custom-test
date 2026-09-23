#pragma once

#include <cstddef>
#include <limits>

class CursorModel;
class MarkerModel;
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

enum class ChartDragKind {
    None,
    PanX,
    PanY,
    CursorA,
    CursorB,
    Marker,
    MarkerCandidate,
};

struct ChartInteractionState {
    ChartDragKind dragKind = ChartDragKind::None;
    std::size_t markerIndex = std::numeric_limits<std::size_t>::max();
    std::size_t seriesIndex = std::numeric_limits<std::size_t>::max();
    float pressMouseX = 0.0F;
    float pressMouseY = 0.0F;
    double pressXMin = 0.0;
    double pressXMax = 0.0;
    float pressYMin = 0.0F;
    float pressYMax = 0.0F;
    bool markerCandidateCancelled = false;

    void reset() { *this = {}; }
};

class Chart {
public:
    static void draw(Dataset& dataset,
                     std::size_t& activeSeriesIndex,
                     ActiveHighlightMode activeHighlightMode,
                     double& xMin,
                     double& xMax,
                     bool showCrosshair,
                     ChartMouseState& mouseState,
                     ChartLayoutState& layoutState,
                     CursorModel& cursorModel,
                     MarkerModel& markerModel,
                     ChartInteractionState& interactionState);
};
