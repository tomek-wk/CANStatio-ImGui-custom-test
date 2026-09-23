#include "Chart.h"

#include "ChartMath.h"
#include "CursorModel.h"
#include "Dataset.h"
#include "InputPolicy.h"
#include "MarkerModel.h"
#include "SeriesStyle.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>

namespace {
constexpr float kLeftMargin = 96.0F;
constexpr float kRightMargin = 16.0F;
constexpr float kTopMargin = 24.0F;
constexpr float kBottomMargin = 58.0F;
constexpr float kInactiveLineWeight = 1.0F;
constexpr float kActiveLineWeight = 1.5F;
constexpr float kHaloWeight = 7.0F;
constexpr float kHaloAlpha = 0.22F;
constexpr float kOutlineWeight = 5.0F;
constexpr float kOutlineAlpha = 0.65F;
constexpr float kOverlayHitTolerance = 6.0F;
constexpr float kSeriesHitTolerance = 5.0F;
constexpr float kMarkerThreshold = 5.0F;
constexpr std::size_t kNoIndex = std::numeric_limits<std::size_t>::max();

PlotRect makePlotRect(const ImVec2& plotMin, const ImVec2& plotMax) {
    return PlotRect{.left = plotMin.x, .top = plotMin.y, .right = plotMax.x, .bottom = plotMax.y};
}

bool hasActiveVisibleSeries(const Dataset& dataset, std::size_t activeSeriesIndex) {
    return activeSeriesIndex < dataset.series.size() && dataset.series[activeSeriesIndex].visible;
}

void recoverInvalidRanges(Dataset& dataset, double& xMin, double& xMax) {
    if (!ChartMath::validRange(xMin, xMax)) {
        xMin = 0.0;
        xMax = std::max(dataset.endTimeSeconds(), std::max(dataset.dtSeconds, 1.0e-6));
    }
    for (Series& series : dataset.series) {
        if (!ChartMath::validRange(series.viewMin, series.viewMax)) {
            series.fitViewToData();
        }
    }
}

std::pair<std::size_t, std::size_t> visibleSampleRange(const Dataset& dataset,
                                                       const Series& series,
                                                       double xMin,
                                                       double xMax) {
    if (series.values.size() < 2 || !(dataset.dtSeconds > 0.0) || !ChartMath::validRange(xMin, xMax)) {
        return {kNoIndex, kNoIndex};
    }

    const double dataEnd = static_cast<double>(series.values.size() - 1) * dataset.dtSeconds;
    if (xMax < 0.0 || xMin > dataEnd) {
        return {kNoIndex, kNoIndex};
    }

    const double clippedMin = std::max(0.0, xMin);
    const double clippedMax = std::min(dataEnd, xMax);
    long long first = static_cast<long long>(std::floor(clippedMin / dataset.dtSeconds)) - 1;
    long long last = static_cast<long long>(std::ceil(clippedMax / dataset.dtSeconds)) + 1;
    const long long maxIndex = static_cast<long long>(series.values.size()) - 1;
    first = std::clamp(first, 0LL, maxIndex);
    last = std::clamp(last, 0LL, maxIndex);
    if (last <= first) {
        return {kNoIndex, kNoIndex};
    }
    return {static_cast<std::size_t>(first), static_cast<std::size_t>(last)};
}

std::vector<ImVec2> makeSeriesPoints(const Dataset& dataset,
                                     const Series& series,
                                     double xMin,
                                     double xMax,
                                     const PlotRect& rect) {
    std::vector<ImVec2> points;
    if (!series.visible || !ChartMath::validRange(series.viewMin, series.viewMax) || !rect.valid()) {
        return points;
    }

    const auto [first, last] = visibleSampleRange(dataset, series, xMin, xMax);
    if (first == kNoIndex) {
        return points;
    }

    points.reserve(last - first + 1);
    for (std::size_t i = first; i <= last; ++i) {
        const double time = static_cast<double>(i) * dataset.dtSeconds;
        points.emplace_back(
            static_cast<float>(ChartMath::dataToScreenX(time, xMin, xMax, rect)),
            static_cast<float>(ChartMath::dataToScreenY(series.values[i], series.viewMin, series.viewMax, rect)));
    }
    return points;
}

void drawPolyline(ImDrawList* drawList,
                  const std::vector<ImVec2>& points,
                  const ImVec4& color,
                  float weight) {
    if (points.size() < 2) {
        return;
    }
    drawList->AddPolyline(points.data(),
                          static_cast<int>(points.size()),
                          ImGui::ColorConvertFloat4ToU32(color),
                          ImDrawFlags_None,
                          weight);
}

void drawInactiveSeries(ImDrawList* drawList,
                        const Dataset& dataset,
                        const Series& series,
                        std::size_t seriesIndex,
                        double xMin,
                        double xMax,
                        const PlotRect& rect) {
    drawPolyline(drawList,
                 makeSeriesPoints(dataset, series, xMin, xMax, rect),
                 SeriesStyle::color(seriesIndex),
                 kInactiveLineWeight);
}

void drawActiveSeries(ImDrawList* drawList,
                      const Dataset& dataset,
                      const Series& series,
                      std::size_t seriesIndex,
                      ActiveHighlightMode mode,
                      double xMin,
                      double xMax,
                      const PlotRect& rect) {
    const std::vector<ImVec2> points = makeSeriesPoints(dataset, series, xMin, xMax, rect);
    if (points.size() < 2) {
        return;
    }

    ImVec4 highlightColor;
    float highlightWeight = 0.0F;
    if (mode == ActiveHighlightMode::Halo) {
        highlightColor = SeriesStyle::color(seriesIndex);
        highlightColor.w *= kHaloAlpha;
        highlightWeight = kHaloWeight;
    } else {
        highlightColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        highlightColor.w *= kOutlineAlpha;
        highlightWeight = kOutlineWeight;
    }

    drawPolyline(drawList, points, highlightColor, highlightWeight);
    drawPolyline(drawList, points, SeriesStyle::color(seriesIndex), kActiveLineWeight);
}

std::string formatTimeLabel(double seconds, double tickStep) {
    const bool negative = seconds < 0.0;
    const double absolute = std::abs(seconds);
    const long long totalMilliseconds = std::llround(absolute * 1000.0);
    const long long milliseconds = totalMilliseconds % 1000;
    const long long totalSeconds = totalMilliseconds / 1000;
    const long long secondsPart = totalSeconds % 60;
    const long long totalMinutes = totalSeconds / 60;
    const long long minutesPart = totalMinutes % 60;
    const long long hours = totalMinutes / 60;
    const char* sign = negative ? "-" : "";

    char buffer[64]{};
    if (tickStep < 1.0) {
        if (hours > 0) {
            std::snprintf(buffer, sizeof(buffer), "%s%02lld:%02lld:%02lld.%03lld", sign, hours, minutesPart, secondsPart, milliseconds);
        } else {
            std::snprintf(buffer, sizeof(buffer), "%s%02lld:%02lld.%03lld", sign, totalMinutes, secondsPart, milliseconds);
        }
    } else if (hours > 0) {
        std::snprintf(buffer, sizeof(buffer), "%s%02lld:%02lld:%02lld", sign, hours, minutesPart, secondsPart);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%s%02lld:%02lld", sign, totalMinutes, secondsPart);
    }
    return buffer;
}

std::string formatYLabel(double value, double step) {
    char buffer[64]{};
    const double magnitude = std::max(std::abs(value), std::abs(step));
    if ((magnitude > 0.0 && magnitude < 1.0e-4) || magnitude >= 1.0e7) {
        std::snprintf(buffer, sizeof(buffer), "%.3e", value);
        return buffer;
    }

    int decimals = 2;
    if (step > 0.0 && step < 0.01) {
        decimals = std::clamp(static_cast<int>(std::ceil(-std::log10(step))) + 1, 2, 6);
    }
    std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
    return buffer;
}

void drawAxesAndGrid(ImDrawList* drawList,
                     const Dataset& dataset,
                     std::size_t activeSeriesIndex,
                     double xMin,
                     double xMax,
                     const PlotRect& rect) {
    ImVec4 gridColor = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
    gridColor.w *= 0.38F;
    const ImU32 neutralGrid = ImGui::ColorConvertFloat4ToU32(gridColor);

    const std::vector<double> xTicks = ChartMath::makeTimeTicks(xMin, xMax, rect.width());
    double xStep = xMax - xMin;
    if (xTicks.size() >= 2) {
        xStep = xTicks[1] - xTicks[0];
    }

    for (double tick : xTicks) {
        const float x = static_cast<float>(ChartMath::dataToScreenX(tick, xMin, xMax, rect));
        if (x < rect.left - 0.5 || x > rect.right + 0.5) {
            continue;
        }
        drawList->AddLine(ImVec2(x, static_cast<float>(rect.top)),
                          ImVec2(x, static_cast<float>(rect.bottom)),
                          neutralGrid,
                          1.0F);
        const std::string label = formatTimeLabel(tick, xStep);
        const ImVec2 size = ImGui::CalcTextSize(label.c_str());
        drawList->AddText(ImVec2(x - 0.5F * size.x, static_cast<float>(rect.bottom) + 8.0F),
                          ImGui::GetColorU32(ImGuiCol_TextDisabled),
                          label.c_str());
    }

    const int horizontalCount = std::max(1, static_cast<int>(std::floor(rect.height() / 70.0)));
    for (int i = 1; i < horizontalCount; ++i) {
        const float y = static_cast<float>(rect.top + rect.height() * static_cast<double>(i) / horizontalCount);
        drawList->AddLine(ImVec2(static_cast<float>(rect.left), y),
                          ImVec2(static_cast<float>(rect.right), y),
                          neutralGrid,
                          1.0F);
    }

    if (!hasActiveVisibleSeries(dataset, activeSeriesIndex)) {
        return;
    }

    const Series& series = dataset.series[activeSeriesIndex];
    const std::vector<double> yTicks = ChartMath::makeNiceTicks(series.viewMin, series.viewMax, rect.height());
    double yStep = static_cast<double>(series.viewMax) - series.viewMin;
    if (yTicks.size() >= 2) {
        yStep = yTicks[1] - yTicks[0];
    }

    ImVec4 semanticColor = SeriesStyle::color(activeSeriesIndex);
    ImVec4 semanticGridColor = semanticColor;
    semanticGridColor.w *= 0.30F;
    const ImU32 semanticGrid = ImGui::ColorConvertFloat4ToU32(semanticGridColor);
    const ImU32 semanticText = ImGui::ColorConvertFloat4ToU32(semanticColor);

    for (double tick : yTicks) {
        const float y = static_cast<float>(ChartMath::dataToScreenY(tick, series.viewMin, series.viewMax, rect));
        if (y < rect.top - 0.5 || y > rect.bottom + 0.5) {
            continue;
        }
        drawList->AddLine(ImVec2(static_cast<float>(rect.left), y),
                          ImVec2(static_cast<float>(rect.right), y),
                          semanticGrid,
                          1.0F);
        const std::string label = formatYLabel(tick, yStep);
        const ImVec2 size = ImGui::CalcTextSize(label.c_str());
        drawList->AddText(ImVec2(static_cast<float>(rect.left) - size.x - 8.0F, y - 0.5F * size.y),
                          semanticText,
                          label.c_str());
    }

    drawList->AddText(ImVec2(static_cast<float>(rect.left) + 6.0F, static_cast<float>(rect.top) + 5.0F),
                      semanticText,
                      series.name.c_str());
}

void updateMouseState(const Dataset& dataset,
                      double xMin,
                      double xMax,
                      const PlotRect& rect,
                      bool plotHovered,
                      ChartMouseState& mouseState) {
    mouseState = {};
    mouseState.plotHovered = plotHovered;
    if (!plotHovered || !ChartMath::validRange(xMin, xMax) || !rect.valid()) {
        return;
    }

    mouseState.timeSeconds = ChartMath::screenToDataX(ImGui::GetMousePos().x, xMin, xMax, rect);
    mouseState.withinDataset = dataset.sampleCount > 0
        && mouseState.timeSeconds >= 0.0
        && mouseState.timeSeconds <= dataset.endTimeSeconds();
}

struct OverlayHit {
    PointerTarget target = PointerTarget::None;
    std::size_t markerIndex = kNoIndex;
};

OverlayHit hitOverlay(const CursorModel& cursorModel,
                      const MarkerModel& markerModel,
                      double xMin,
                      double xMax,
                      const PlotRect& rect,
                      float mouseX) {
    OverlayHit hit;
    double bestDistance = kOverlayHitTolerance + 1.0;

    auto consider = [&](PointerTarget target, double time, std::size_t markerIndex) {
        const double x = ChartMath::dataToScreenX(time, xMin, xMax, rect);
        const double distance = std::abs(static_cast<double>(mouseX) - x);
        if (x >= rect.left - kOverlayHitTolerance && x <= rect.right + kOverlayHitTolerance
            && distance <= kOverlayHitTolerance && distance < bestDistance) {
            bestDistance = distance;
            hit.target = target;
            hit.markerIndex = markerIndex;
        }
    };

    if (cursorModel.a().visible) {
        consider(PointerTarget::CursorA, cursorModel.a().timeSeconds, kNoIndex);
    }
    if (cursorModel.b().visible) {
        consider(PointerTarget::CursorB, cursorModel.b().timeSeconds, kNoIndex);
    }
    const auto& markers = markerModel.markers();
    for (std::size_t i = 0; i < markers.size(); ++i) {
        consider(PointerTarget::Marker, markers[i].timeSeconds, i);
    }
    return hit;
}

std::size_t hitSeries(const Dataset& dataset,
                      double xMin,
                      double xMax,
                      const PlotRect& rect,
                      const ImVec2& mouse) {
    if (!rect.valid() || !ChartMath::validRange(xMin, xMax) || !(dataset.dtSeconds > 0.0)) {
        return kNoIndex;
    }

    const double mouseTime = ChartMath::screenToDataX(mouse.x, xMin, xMax, rect);
    const double toleranceTime = kSeriesHitTolerance * (xMax - xMin) / rect.width();
    double bestDistanceSquared = static_cast<double>(kSeriesHitTolerance * kSeriesHitTolerance);
    std::size_t bestSeries = kNoIndex;

    for (std::size_t s = 0; s < dataset.series.size(); ++s) {
        const Series& series = dataset.series[s];
        if (!series.visible || series.values.size() < 2 || !ChartMath::validRange(series.viewMin, series.viewMax)) {
            continue;
        }

        const double dataEnd = static_cast<double>(series.values.size() - 1) * dataset.dtSeconds;
        if (mouseTime + toleranceTime < 0.0 || mouseTime - toleranceTime > dataEnd) {
            continue;
        }

        long long first = static_cast<long long>(std::floor((mouseTime - toleranceTime) / dataset.dtSeconds)) - 1;
        long long last = static_cast<long long>(std::ceil((mouseTime + toleranceTime) / dataset.dtSeconds)) + 1;
        const long long maxIndex = static_cast<long long>(series.values.size()) - 1;
        first = std::clamp(first, 0LL, maxIndex - 1);
        last = std::clamp(last, 1LL, maxIndex);

        for (long long i = first; i < last; ++i) {
            const std::size_t aIndex = static_cast<std::size_t>(i);
            const std::size_t bIndex = aIndex + 1;
            const double ax = ChartMath::dataToScreenX(static_cast<double>(aIndex) * dataset.dtSeconds, xMin, xMax, rect);
            const double bx = ChartMath::dataToScreenX(static_cast<double>(bIndex) * dataset.dtSeconds, xMin, xMax, rect);
            const double ay = ChartMath::dataToScreenY(series.values[aIndex], series.viewMin, series.viewMax, rect);
            const double by = ChartMath::dataToScreenY(series.values[bIndex], series.viewMin, series.viewMax, rect);
            const double distanceSquared = ChartMath::pointSegmentDistanceSquared(mouse.x, mouse.y, ax, ay, bx, by);
            if (distanceSquared <= bestDistanceSquared) {
                bestDistanceSquared = distanceSquared;
                bestSeries = s;
            }
        }
    }
    return bestSeries;
}

void handleActiveDrag(Dataset& dataset,
                      double& xMin,
                      double& xMax,
                      const PlotRect& rect,
                      CursorModel& cursorModel,
                      MarkerModel& markerModel,
                      ChartInteractionState& state) {
    if (state.dragKind == ChartDragKind::None) {
        return;
    }

    const ImVec2 mouse = ImGui::GetMousePos();
    const bool released = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    const bool activeFrame = ImGui::IsMouseDown(ImGuiMouseButton_Left) || released;
    if (activeFrame) {
        switch (state.dragKind) {
        case ChartDragKind::PanX:
            xMin = state.pressXMin;
            xMax = state.pressXMax;
            ChartMath::panX(xMin, xMax, mouse.x - state.pressMouseX, rect.width());
            break;
        case ChartDragKind::PanY:
            if (state.seriesIndex < dataset.series.size()) {
                Series& series = dataset.series[state.seriesIndex];
                series.viewMin = state.pressYMin;
                series.viewMax = state.pressYMax;
                ChartMath::panY(series.viewMin, series.viewMax, mouse.y - state.pressMouseY, rect.height());
            }
            break;
        case ChartDragKind::CursorA:
            cursorModel.setA(ChartMath::screenToDataX(mouse.x, xMin, xMax, rect), dataset.endTimeSeconds());
            break;
        case ChartDragKind::CursorB:
            cursorModel.setB(ChartMath::screenToDataX(mouse.x, xMin, xMax, rect), dataset.endTimeSeconds());
            break;
        case ChartDragKind::Marker:
            markerModel.move(state.markerIndex,
                             ChartMath::screenToDataX(mouse.x, xMin, xMax, rect),
                             dataset.endTimeSeconds());
            break;
        case ChartDragKind::MarkerCandidate: {
            const double dx = mouse.x - state.pressMouseX;
            const double dy = mouse.y - state.pressMouseY;
            if (!state.markerCandidateCancelled && dx * dx + dy * dy > kMarkerThreshold * kMarkerThreshold) {
                if (state.seriesIndex < dataset.series.size() && dataset.series[state.seriesIndex].visible) {
                    state.dragKind = ChartDragKind::PanY;
                    Series& series = dataset.series[state.seriesIndex];
                    series.viewMin = state.pressYMin;
                    series.viewMax = state.pressYMax;
                    ChartMath::panY(series.viewMin, series.viewMax, dy, rect.height());
                } else {
                    state.markerCandidateCancelled = true;
                }
            }
            break;
        }
        case ChartDragKind::None:
            break;
        }
    }

    if (released) {
        if (state.dragKind == ChartDragKind::MarkerCandidate && !state.markerCandidateCancelled) {
            markerModel.add(ChartMath::screenToDataX(mouse.x, xMin, xMax, rect), dataset.endTimeSeconds());
        }
        state.reset();
    }
}

void beginLeftAction(Dataset& dataset,
                     std::size_t activeSeriesIndex,
                     double xMin,
                     double xMax,
                     const PlotRect& rect,
                     CursorModel& cursorModel,
                     ChartInteractionState& state,
                     const OverlayHit& overlayHit) {
    const ImGuiIO& io = ImGui::GetIO();
    const ImVec2 mouse = ImGui::GetMousePos();
    const double mouseTime = ChartMath::screenToDataX(mouse.x, xMin, xMax, rect);
    const LeftPressAction action = InputPolicy::resolveLeftPress(io.KeyShift, io.KeyCtrl, io.KeyAlt, overlayHit.target);

    switch (action) {
    case LeftPressAction::SetCursorA:
        cursorModel.setA(mouseTime, dataset.endTimeSeconds());
        break;
    case LeftPressAction::SetCursorB:
        cursorModel.setB(mouseTime, dataset.endTimeSeconds());
        break;
    case LeftPressAction::MarkerCandidate:
        state.reset();
        state.dragKind = ChartDragKind::MarkerCandidate;
        state.pressMouseX = mouse.x;
        state.pressMouseY = mouse.y;
        if (hasActiveVisibleSeries(dataset, activeSeriesIndex)) {
            state.seriesIndex = activeSeriesIndex;
            state.pressYMin = dataset.series[activeSeriesIndex].viewMin;
            state.pressYMax = dataset.series[activeSeriesIndex].viewMax;
        }
        break;
    case LeftPressAction::DragCursorA:
        state.reset();
        state.dragKind = ChartDragKind::CursorA;
        break;
    case LeftPressAction::DragCursorB:
        state.reset();
        state.dragKind = ChartDragKind::CursorB;
        break;
    case LeftPressAction::DragMarker:
        state.reset();
        state.dragKind = ChartDragKind::Marker;
        state.markerIndex = overlayHit.markerIndex;
        break;
    case LeftPressAction::PanX:
        state.reset();
        state.dragKind = ChartDragKind::PanX;
        state.pressMouseX = mouse.x;
        state.pressMouseY = mouse.y;
        state.pressXMin = xMin;
        state.pressXMax = xMax;
        break;
    }
}

void handleNewInput(Dataset& dataset,
                    std::size_t& activeSeriesIndex,
                    double& xMin,
                    double& xMax,
                    const PlotRect& rect,
                    bool plotHovered,
                    CursorModel& cursorModel,
                    MarkerModel& markerModel,
                    ChartInteractionState& state) {
    if (!plotHovered || state.dragKind != ChartDragKind::None || !rect.valid()) {
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    const ImVec2 mouse = ImGui::GetMousePos();
    const OverlayHit overlayHit = hitOverlay(cursorModel, markerModel, xMin, xMax, rect, mouse.x);

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        beginLeftAction(dataset,
                        activeSeriesIndex,
                        xMin,
                        xMax,
                        rect,
                        cursorModel,
                        state,
                        overlayHit);
        if (state.dragKind == ChartDragKind::Marker && overlayHit.markerIndex == kNoIndex) {
            state.reset();
        }
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        const RightPressAction action = InputPolicy::resolveRightPress(io.KeyShift, io.KeyCtrl, io.KeyAlt, overlayHit.target);
        switch (action) {
        case RightPressAction::HideCursorA:
            cursorModel.hideA();
            break;
        case RightPressAction::HideCursorB:
            cursorModel.hideB();
            break;
        case RightPressAction::RemoveMarker:
            markerModel.remove(overlayHit.markerIndex);
            break;
        case RightPressAction::SelectSeries: {
            const std::size_t hit = hitSeries(dataset, xMin, xMax, rect, mouse);
            activeSeriesIndex = (hit == activeSeriesIndex) ? kNoIndex : hit;
            break;
        }
        case RightPressAction::None:
            break;
        }
    }

    if (io.MouseWheel != 0.0F) {
        const bool activeVisible = hasActiveVisibleSeries(dataset, activeSeriesIndex);
        const WheelAction action = InputPolicy::resolveWheel(io.KeyShift, io.KeyCtrl, io.KeyAlt, activeVisible);
        if (action == WheelAction::ZoomX) {
            const double anchorFraction = std::clamp((static_cast<double>(mouse.x) - rect.left) / rect.width(), 0.0, 1.0);
            ChartMath::zoomX(xMin,
                             xMax,
                             anchorFraction,
                             io.MouseWheel,
                             std::max(dataset.dtSeconds, 1.0e-9));
        } else if (action == WheelAction::ZoomY && activeVisible) {
            Series& series = dataset.series[activeSeriesIndex];
            ChartMath::zoomY(series.viewMin, series.viewMax, io.MouseWheel);
        }
    }
}

void drawVerticalOverlay(ImDrawList* drawList,
                         double time,
                         double xMin,
                         double xMax,
                         const PlotRect& rect,
                         ImU32 color,
                         float weight,
                         const std::string& label,
                         float labelYOffset) {
    const float x = static_cast<float>(ChartMath::dataToScreenX(time, xMin, xMax, rect));
    if (x < rect.left || x > rect.right) {
        return;
    }
    drawList->AddLine(ImVec2(x, static_cast<float>(rect.top)),
                      ImVec2(x, static_cast<float>(rect.bottom)),
                      color,
                      weight);
    drawList->AddText(ImVec2(x + 3.0F, static_cast<float>(rect.top) + labelYOffset), color, label.c_str());
}

void drawOverlays(ImDrawList* drawList,
                  const CursorModel& cursorModel,
                  const MarkerModel& markerModel,
                  double xMin,
                  double xMax,
                  const PlotRect& rect,
                  bool showCrosshair,
                  bool plotHovered) {
    if (showCrosshair && plotHovered) {
        ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        color.w *= 0.45F;
        const float x = ImGui::GetMousePos().x;
        drawList->AddLine(ImVec2(x, static_cast<float>(rect.top)),
                          ImVec2(x, static_cast<float>(rect.bottom)),
                          ImGui::ColorConvertFloat4ToU32(color),
                          1.0F);
    }

    const ImU32 markerColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0F, 0.55F, 0.18F, 0.95F));
    for (const TimeMarker& marker : markerModel.markers()) {
        drawVerticalOverlay(drawList,
                            marker.timeSeconds,
                            xMin,
                            xMax,
                            rect,
                            markerColor,
                            1.0F,
                            std::to_string(marker.id),
                            21.0F);
    }

    if (cursorModel.a().visible) {
        drawVerticalOverlay(drawList,
                            cursorModel.a().timeSeconds,
                            xMin,
                            xMax,
                            rect,
                            ImGui::ColorConvertFloat4ToU32(ImVec4(1.0F, 0.86F, 0.20F, 1.0F)),
                            2.0F,
                            "A",
                            3.0F);
    }
    if (cursorModel.b().visible) {
        drawVerticalOverlay(drawList,
                            cursorModel.b().timeSeconds,
                            xMin,
                            xMax,
                            rect,
                            ImGui::ColorConvertFloat4ToU32(ImVec4(0.20F, 0.88F, 1.0F, 1.0F)),
                            2.0F,
                            "B",
                            3.0F);
    }
}
} // namespace

void Chart::draw(Dataset& dataset,
                 std::size_t& activeSeriesIndex,
                 ActiveHighlightMode activeHighlightMode,
                 double& xMin,
                 double& xMax,
                 bool showCrosshair,
                 ChartMouseState& mouseState,
                 ChartLayoutState& layoutState,
                 CursorModel& cursorModel,
                 MarkerModel& markerModel,
                 ChartInteractionState& interactionState) {
    mouseState = {};
    layoutState = {};
    recoverInvalidRanges(dataset, xMin, xMax);

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    constexpr ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));
    ImGui::Begin("ChartHost", nullptr, windowFlags);
    ImGui::PopStyleVar();

    const ImVec2 canvasMin = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    const float plotWidth = canvasSize.x - kLeftMargin - kRightMargin;
    const float plotHeight = canvasSize.y - kTopMargin - kBottomMargin;
    if (!(plotWidth > 0.0F) || !(plotHeight > 0.0F)) {
        interactionState.reset();
        ImGui::End();
        return;
    }

    const ImVec2 plotMin(canvasMin.x + kLeftMargin, canvasMin.y + kTopMargin);
    const ImVec2 plotMax(plotMin.x + plotWidth, plotMin.y + plotHeight);
    const PlotRect rect = makePlotRect(plotMin, plotMax);

    layoutState.plotX = plotMin.x;
    layoutState.plotY = plotMin.y;
    layoutState.plotWidth = plotWidth;
    layoutState.plotHeight = plotHeight;

    ImGui::SetCursorScreenPos(plotMin);
    ImGui::InvisibleButton("##CustomPlotInput",
                           ImVec2(plotWidth, plotHeight),
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    const bool plotHovered = ImGui::IsItemHovered();

    handleActiveDrag(dataset,
                     xMin,
                     xMax,
                     rect,
                     cursorModel,
                     markerModel,
                     interactionState);
    handleNewInput(dataset,
                   activeSeriesIndex,
                   xMin,
                   xMax,
                   rect,
                   plotHovered,
                   cursorModel,
                   markerModel,
                   interactionState);
    updateMouseState(dataset, xMin, xMax, rect, plotHovered, mouseState);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(plotMin, plotMax, ImGui::GetColorU32(ImGuiCol_FrameBg));
    drawAxesAndGrid(drawList, dataset, activeSeriesIndex, xMin, xMax, rect);

    drawList->PushClipRect(plotMin, plotMax, true);
    for (std::size_t i = 0; i < dataset.series.size(); ++i) {
        if (i == activeSeriesIndex) {
            continue;
        }
        drawInactiveSeries(drawList, dataset, dataset.series[i], i, xMin, xMax, rect);
    }

    if (hasActiveVisibleSeries(dataset, activeSeriesIndex)) {
        drawActiveSeries(drawList,
                         dataset,
                         dataset.series[activeSeriesIndex],
                         activeSeriesIndex,
                         activeHighlightMode,
                         xMin,
                         xMax,
                         rect);
    }

    drawOverlays(drawList,
                 cursorModel,
                 markerModel,
                 xMin,
                 xMax,
                 rect,
                 showCrosshair,
                 plotHovered);
    drawList->PopClipRect();

    drawList->AddRect(plotMin,
                      plotMax,
                      ImGui::GetColorU32(ImGuiCol_Border),
                      0.0F,
                      ImDrawFlags_None,
                      1.0F);

    ImGui::End();
}
