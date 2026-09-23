#include "Chart.h"

#include "CursorOverlay.h"
#include "Dataset.h"
#include "SeriesStyle.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

#include <imgui.h>
#include <implot.h>

namespace {
constexpr float kSeriesHitTolerancePixels = 5.0F;
constexpr float kActiveLineWeightPixels = 1.5F;
constexpr float kHaloWeightPixels = 7.0F;
constexpr float kHaloAlpha = 0.22F;
constexpr float kOutlineWeightPixels = 5.0F;
constexpr float kOutlineAlpha = 0.65F;
constexpr float kYAxisTickLengthPixels = 5.0F;
constexpr float kYAxisLabelGapPixels = 5.0F;
constexpr float kYAxisExtraPaddingPixels = 6.0F;
constexpr const char* kYAxisReferenceLabel = "-0.000e+00";

struct PlotSeriesContext {
    const Series* series = nullptr;
    double dtSeconds = 0.0;
};

struct YTickData {
    std::vector<double> positions;
    std::vector<double> rawValues;
    std::vector<std::string> labels;
};

struct XTickData {
    std::vector<double> positions;
    std::vector<std::string> labels;
    std::vector<const char*> labelPointers;
};

ImPlotPoint normalizedPointGetter(int index, void* userData) {
    const auto* context = static_cast<const PlotSeriesContext*>(userData);
    const Series& series = *context->series;

    const double x = static_cast<double>(index) * context->dtSeconds;
    const double span = static_cast<double>(series.viewMax - series.viewMin);
    const double y = (static_cast<double>(series.values[static_cast<std::size_t>(index)]) - series.viewMin) / span;
    return ImPlotPoint(x, y);
}

void plotSeries(const Dataset& dataset, const Series& series, std::size_t seriesIndex, float lineWeight) {
    if (!series.visible || series.values.empty()) {
        return;
    }

    PlotSeriesContext context{.series = &series, .dtSeconds = dataset.dtSeconds};
    ImPlotSpec spec;
    spec.LineColor = SeriesStyle::color(seriesIndex);
    spec.LineWeight = lineWeight;

    ImPlot::PlotLineG(series.name.c_str(),
                      normalizedPointGetter,
                      &context,
                      static_cast<int>(series.values.size()),
                      spec);
}

void plotActiveHighlight(const Dataset& dataset,
                         const Series& series,
                         std::size_t seriesIndex,
                         ActiveHighlightMode mode) {
    if (!series.visible || series.values.size() < 2 || !(dataset.dtSeconds > 0.0)) {
        return;
    }

    const double ySpan = static_cast<double>(series.viewMax - series.viewMin);
    if (!(ySpan > 0.0) || !std::isfinite(ySpan)) {
        return;
    }

    ImVec4 highlightColor;
    float highlightWeight = 0.0F;

    if (mode == ActiveHighlightMode::Halo) {
        highlightColor = SeriesStyle::color(seriesIndex);
        highlightColor.w *= kHaloAlpha;
        highlightWeight = kHaloWeightPixels;
    } else {
        highlightColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        highlightColor.w *= kOutlineAlpha;
        highlightWeight = kOutlineWeightPixels;
    }

    const ImPlotRect limits = ImPlot::GetPlotLimits(ImAxis_X1, ImAxis_Y1);
    const long long maxIndex = static_cast<long long>(series.values.size()) - 1;
    long long firstIndex = static_cast<long long>(std::floor(limits.X.Min / dataset.dtSeconds)) - 1;
    long long lastIndex = static_cast<long long>(std::ceil(limits.X.Max / dataset.dtSeconds)) + 1;
    firstIndex = std::clamp(firstIndex, 0LL, maxIndex);
    lastIndex = std::clamp(lastIndex, 0LL, maxIndex);

    if (lastIndex <= firstIndex) {
        return;
    }

    std::vector<ImVec2> points;
    points.reserve(static_cast<std::size_t>(lastIndex - firstIndex + 1));
    for (long long index = firstIndex; index <= lastIndex; ++index) {
        const std::size_t sample = static_cast<std::size_t>(index);
        const double x = static_cast<double>(sample) * dataset.dtSeconds;
        const double y = (static_cast<double>(series.values[sample]) - series.viewMin) / ySpan;
        points.push_back(ImPlot::PlotToPixels(x, y, ImAxis_X1, ImAxis_Y1));
    }

    ImPlot::PushPlotClipRect();
    ImPlot::GetPlotDrawList()->AddPolyline(points.data(),
                                           static_cast<int>(points.size()),
                                           ImGui::ColorConvertFloat4ToU32(highlightColor),
                                           ImDrawFlags_None,
                                           highlightWeight);
    ImPlot::PopPlotClipRect();
}

double niceStep(double roughStep) {
    if (!(roughStep > 0.0) || !std::isfinite(roughStep)) {
        return 1.0;
    }

    const double exponent = std::floor(std::log10(roughStep));
    const double magnitude = std::pow(10.0, exponent);
    const double fraction = roughStep / magnitude;

    double niceFraction = 1.0;
    if (fraction <= 1.0) {
        niceFraction = 1.0;
    } else if (fraction <= 2.0) {
        niceFraction = 2.0;
    } else if (fraction <= 5.0) {
        niceFraction = 5.0;
    } else {
        niceFraction = 10.0;
    }

    return niceFraction * magnitude;
}

YTickData makeYTicks(const Series& series, float plotHeight) {
    YTickData ticks;

    const double viewMin = static_cast<double>(series.viewMin);
    const double viewMax = static_cast<double>(series.viewMax);
    const double span = viewMax - viewMin;
    if (!(span > 0.0) || !std::isfinite(span)) {
        return ticks;
    }

    const int targetTickCount = std::clamp(static_cast<int>(plotHeight / 75.0F), 3, 10);
    const double step = niceStep(span / static_cast<double>(targetTickCount - 1));
    const double first = std::ceil(viewMin / step) * step;
    const double epsilon = step * 1.0e-6;

    const int decimals = (static_cast<double>(series.dataMax) - series.dataMin) < 1.0 ? 3 : 2;

    for (double raw = first; raw <= viewMax + epsilon; raw += step) {
        if (raw < viewMin - epsilon) {
            continue;
        }

        const double normalized = (raw - viewMin) / span;
        if (normalized < -1.0e-6 || normalized > 1.0 + 1.0e-6) {
            continue;
        }

        ticks.positions.push_back(std::clamp(normalized, 0.0, 1.0));
        ticks.rawValues.push_back(raw);

        char buffer[64]{};
        std::snprintf(buffer, sizeof(buffer), decimals == 3 ? "%.3f" : "%.2f", raw);
        ticks.labels.emplace_back(buffer);
    }

    return ticks;
}

double chooseTimeStep(double roughStep) {
    constexpr double timeSteps[] = {
        0.001, 0.002, 0.005,
        0.010, 0.020, 0.050,
        0.100, 0.200, 0.500,
        1.0, 2.0, 5.0, 10.0, 15.0, 30.0,
        60.0, 120.0, 300.0, 600.0, 900.0, 1800.0,
        3600.0, 7200.0, 14400.0, 21600.0, 43200.0, 86400.0,
    };

    for (const double step : timeSteps) {
        if (step >= roughStep) {
            return step;
        }
    }

    return niceStep(roughStep);
}

std::string formatTimeLabel(double seconds, double stepSeconds, double maxAbsTime) {
    const bool negative = seconds < -1.0e-9;
    const char* sign = negative ? "-" : "";
    const double absoluteSeconds = std::abs(seconds);
    char buffer[64]{};

    if (stepSeconds < 1.0) {
        const long long totalMilliseconds = std::llround(absoluteSeconds * 1000.0);
        const long long milliseconds = totalMilliseconds % 1000;
        const long long totalSeconds = totalMilliseconds / 1000;
        const long long secondsPart = totalSeconds % 60;
        const long long totalMinutes = totalSeconds / 60;
        const long long minutesPart = totalMinutes % 60;
        const long long hours = totalMinutes / 60;

        if (maxAbsTime >= 3600.0) {
            std::snprintf(buffer,
                          sizeof(buffer),
                          "%s%02lld:%02lld:%02lld.%03lld",
                          sign,
                          hours,
                          minutesPart,
                          secondsPart,
                          milliseconds);
        } else if (maxAbsTime >= 60.0) {
            std::snprintf(buffer,
                          sizeof(buffer),
                          "%s%02lld:%02lld.%03lld",
                          sign,
                          totalMinutes,
                          secondsPart,
                          milliseconds);
        } else {
            std::snprintf(buffer,
                          sizeof(buffer),
                          "%s%02lld.%03lld",
                          sign,
                          totalSeconds,
                          milliseconds);
        }
    } else {
        const long long totalSeconds = std::llround(absoluteSeconds);
        const long long secondsPart = totalSeconds % 60;
        const long long totalMinutes = totalSeconds / 60;
        const long long minutesPart = totalMinutes % 60;
        const long long hours = totalMinutes / 60;

        if (maxAbsTime >= 3600.0) {
            std::snprintf(buffer,
                          sizeof(buffer),
                          "%s%02lld:%02lld:%02lld",
                          sign,
                          hours,
                          minutesPart,
                          secondsPart);
        } else if (maxAbsTime >= 60.0) {
            std::snprintf(buffer,
                          sizeof(buffer),
                          "%s%02lld:%02lld",
                          sign,
                          totalMinutes,
                          secondsPart);
        } else {
            std::snprintf(buffer, sizeof(buffer), "%s%02lld", sign, totalSeconds);
        }
    }

    return buffer;
}

XTickData makeXTicks(double xMin, double xMax, float plotWidth) {
    XTickData ticks;

    const double span = xMax - xMin;
    if (!(span > 0.0) || !std::isfinite(span)) {
        return ticks;
    }

    const int targetIntervals = std::max(2, static_cast<int>(plotWidth / 110.0F));
    const double step = chooseTimeStep(span / static_cast<double>(targetIntervals));
    const double first = std::ceil(xMin / step) * step;
    const double epsilon = step * 1.0e-6;
    const double maxAbsTime = std::max(std::abs(xMin), std::abs(xMax));

    for (double x = first; x <= xMax + epsilon && ticks.positions.size() < 1000; x += step) {
        if (x < xMin - epsilon) {
            continue;
        }

        const double cleanX = std::abs(x) < epsilon ? 0.0 : x;
        ticks.positions.push_back(cleanX);
        ticks.labels.push_back(formatTimeLabel(cleanX, step, maxAbsTime));
    }

    ticks.labelPointers.reserve(ticks.labels.size());
    for (const std::string& label : ticks.labels) {
        ticks.labelPointers.push_back(label.c_str());
    }

    return ticks;
}

float semanticYAxisGutterWidth() {
    return std::ceil(ImGui::CalcTextSize(kYAxisReferenceLabel).x +
                     kYAxisTickLengthPixels +
                     kYAxisLabelGapPixels +
                     kYAxisExtraPaddingPixels);
}

void drawSemanticYAxis(const ChartLayoutState& layoutState,
                       const Series* activeSeries,
                       const YTickData* activeTicks,
                       const ImVec4& activeColor) {
    if (activeSeries == nullptr || activeTicks == nullptr ||
        !(layoutState.plotWidth > 0.0F) || !(layoutState.plotHeight > 0.0F)) {
        return;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImU32 color = ImGui::ColorConvertFloat4ToU32(activeColor);
    const float labelRight = layoutState.plotX - kYAxisTickLengthPixels - kYAxisLabelGapPixels;

    for (std::size_t i = 0; i < activeTicks->positions.size(); ++i) {
        const float normalized = static_cast<float>(activeTicks->positions[i]);
        const float y = layoutState.plotY + (1.0F - normalized) * layoutState.plotHeight;
        const ImVec2 textSize = ImGui::CalcTextSize(activeTicks->labels[i].c_str());
        const float maxTextY = std::max(layoutState.plotY,
                                        layoutState.plotY + layoutState.plotHeight - textSize.y);
        const float textY = std::clamp(y - 0.5F * textSize.y,
                                       layoutState.plotY,
                                       maxTextY);

        drawList->AddLine(ImVec2(layoutState.plotX - kYAxisTickLengthPixels, y),
                          ImVec2(layoutState.plotX, y),
                          color,
                          1.0F);
        drawList->AddText(ImVec2(labelRight - textSize.x, textY),
                          color,
                          activeTicks->labels[i].c_str());
    }
}

void drawHorizontalGrid(const Series* activeSeries,
                        const YTickData* activeTicks,
                        const ImVec4& activeColor) {
    const ImVec2 plotPos = ImPlot::GetPlotPos();
    const ImVec2 plotSize = ImPlot::GetPlotSize();
    ImDrawList* drawList = ImPlot::GetPlotDrawList();

    ImVec4 neutral = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
    neutral.w *= 0.22F;
    const ImU32 neutralColor = ImGui::ColorConvertFloat4ToU32(neutral);

    ImPlot::PushPlotClipRect();
    for (int i = 1; i < 5; ++i) {
        const float normalized = static_cast<float>(i) / 5.0F;
        const float y = plotPos.y + (1.0F - normalized) * plotSize.y;
        drawList->AddLine(ImVec2(plotPos.x, y),
                          ImVec2(plotPos.x + plotSize.x, y),
                          neutralColor,
                          1.0F);
    }

    if (activeSeries != nullptr && activeTicks != nullptr) {
        ImVec4 gridColor = activeColor;
        gridColor.w = 0.35F;
        const ImU32 activeGridColor = ImGui::ColorConvertFloat4ToU32(gridColor);

        for (std::size_t i = 0; i < activeTicks->positions.size(); ++i) {
            const double rawValue = activeTicks->rawValues[i];
            if (rawValue < static_cast<double>(activeSeries->dataMin) ||
                rawValue > static_cast<double>(activeSeries->dataMax)) {
                continue;
            }

            const float normalized = static_cast<float>(activeTicks->positions[i]);
            const float y = plotPos.y + (1.0F - normalized) * plotSize.y;
            drawList->AddLine(ImVec2(plotPos.x, y),
                              ImVec2(plotPos.x + plotSize.x, y),
                              activeGridColor,
                              1.0F);
        }
    }
    ImPlot::PopPlotClipRect();
}

float distanceSquaredToSegment(const ImVec2& point,
                               const ImVec2& start,
                               const ImVec2& end) {
    const float vx = end.x - start.x;
    const float vy = end.y - start.y;
    const float lengthSquared = vx * vx + vy * vy;

    if (lengthSquared <= 1.0e-8F) {
        const float dx = point.x - start.x;
        const float dy = point.y - start.y;
        return dx * dx + dy * dy;
    }

    const float wx = point.x - start.x;
    const float wy = point.y - start.y;
    const float t = std::clamp((wx * vx + wy * vy) / lengthSquared, 0.0F, 1.0F);
    const float closestX = start.x + t * vx;
    const float closestY = start.y + t * vy;
    const float dx = point.x - closestX;
    const float dy = point.y - closestY;
    return dx * dx + dy * dy;
}

std::size_t findSeriesAtMouse(const Dataset& dataset) {
    if (!ImPlot::IsPlotHovered() || !(dataset.dtSeconds > 0.0)) {
        return std::numeric_limits<std::size_t>::max();
    }

    const ImVec2 mouse = ImGui::GetMousePos();
    const ImPlotPoint left = ImPlot::PixelsToPlot(mouse.x - kSeriesHitTolerancePixels,
                                                  mouse.y,
                                                  ImAxis_X1,
                                                  ImAxis_Y1);
    const ImPlotPoint right = ImPlot::PixelsToPlot(mouse.x + kSeriesHitTolerancePixels,
                                                   mouse.y,
                                                   ImAxis_X1,
                                                   ImAxis_Y1);

    const double localXMin = std::min(left.x, right.x);
    const double localXMax = std::max(left.x, right.x);
    const float toleranceSquared = kSeriesHitTolerancePixels * kSeriesHitTolerancePixels;

    std::size_t bestSeries = std::numeric_limits<std::size_t>::max();
    float bestDistanceSquared = toleranceSquared;

    for (std::size_t seriesIndex = 0; seriesIndex < dataset.series.size(); ++seriesIndex) {
        const Series& series = dataset.series[seriesIndex];
        if (!series.visible || series.values.size() < 2) {
            continue;
        }

        const double ySpan = static_cast<double>(series.viewMax - series.viewMin);
        if (!(ySpan > 0.0) || !std::isfinite(ySpan)) {
            continue;
        }

        const long long segmentCount = static_cast<long long>(series.values.size()) - 1;
        long long firstSegment = static_cast<long long>(std::floor(localXMin / dataset.dtSeconds)) - 1;
        long long lastSegment = static_cast<long long>(std::ceil(localXMax / dataset.dtSeconds)) + 1;

        firstSegment = std::clamp(firstSegment, 0LL, segmentCount - 1);
        lastSegment = std::clamp(lastSegment, 0LL, segmentCount - 1);
        if (firstSegment > lastSegment) {
            continue;
        }

        for (long long segment = firstSegment; segment <= lastSegment; ++segment) {
            const std::size_t i0 = static_cast<std::size_t>(segment);
            const std::size_t i1 = i0 + 1;
            const double x0 = static_cast<double>(i0) * dataset.dtSeconds;
            const double x1 = static_cast<double>(i1) * dataset.dtSeconds;
            const double y0 = (static_cast<double>(series.values[i0]) - series.viewMin) / ySpan;
            const double y1 = (static_cast<double>(series.values[i1]) - series.viewMin) / ySpan;

            const ImVec2 p0 = ImPlot::PlotToPixels(x0, y0, ImAxis_X1, ImAxis_Y1);
            const ImVec2 p1 = ImPlot::PlotToPixels(x1, y1, ImAxis_X1, ImAxis_Y1);
            const float distanceSquared = distanceSquaredToSegment(mouse, p0, p1);

            if (distanceSquared <= bestDistanceSquared) {
                bestDistanceSquared = distanceSquared;
                bestSeries = seriesIndex;
            }
        }
    }

    return bestSeries;
}

void handleSeriesSelection(const Dataset& dataset, std::size_t& activeSeriesIndex) {
    const ImGuiIO& io = ImGui::GetIO();
    if (io.KeyMods != ImGuiMod_None || !ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        return;
    }

    if (!ImPlot::IsPlotHovered()) {
        return;
    }

    const std::size_t hitSeries = findSeriesAtMouse(dataset);
    if (hitSeries == activeSeriesIndex) {
        activeSeriesIndex = std::numeric_limits<std::size_t>::max();
    } else {
        activeSeriesIndex = hitSeries;
    }
}

void updateMouseState(const Dataset& dataset, ChartMouseState& mouseState) {
    mouseState = {};
    mouseState.plotHovered = ImPlot::IsPlotHovered();
    if (!mouseState.plotHovered) {
        return;
    }

    const ImPlotPoint mouse = ImPlot::GetPlotMousePos(ImAxis_X1, ImAxis_Y1);
    mouseState.timeSeconds = mouse.x;
    mouseState.withinDataset =
        dataset.sampleCount > 0 &&
        mouse.x >= 0.0 &&
        mouse.x <= dataset.endTimeSeconds();
}

void drawCrosshair(const ChartMouseState& mouseState) {
    if (!mouseState.plotHovered) {
        return;
    }

    const ImVec2 plotPos = ImPlot::GetPlotPos();
    const ImVec2 plotSize = ImPlot::GetPlotSize();
    const ImVec2 pixel = ImPlot::PlotToPixels(mouseState.timeSeconds, 0.0, ImAxis_X1, ImAxis_Y1);

    ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    color.w *= 0.45F;

    ImPlot::PushPlotClipRect();
    ImPlot::GetPlotDrawList()->AddLine(
        ImVec2(pixel.x, plotPos.y),
        ImVec2(pixel.x, plotPos.y + plotSize.y),
        ImGui::ColorConvertFloat4ToU32(color),
        1.0F);
    ImPlot::PopPlotClipRect();
}

void handleActiveYInput(Series& series) {
    if (!ImPlot::IsPlotHovered()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyMods != ImGuiMod_Alt) {
        return;
    }

    const double currentMin = static_cast<double>(series.viewMin);
    const double currentMax = static_cast<double>(series.viewMax);
    const double currentSpan = currentMax - currentMin;
    if (!(currentSpan > 0.0) || !std::isfinite(currentSpan)) {
        return;
    }

    if (io.MouseWheel != 0.0F) {
        const double dataScale = std::max({std::abs(static_cast<double>(series.dataMin)),
                                           std::abs(static_cast<double>(series.dataMax)),
                                           1.0});
        const double minimumSpan = dataScale * 1.0e-7;
        const double zoomFactor = std::pow(0.85, static_cast<double>(io.MouseWheel));
        const double newSpan = std::max(currentSpan * zoomFactor, minimumSpan);
        const double center = 0.5 * (currentMin + currentMax);

        series.viewMin = static_cast<float>(center - 0.5 * newSpan);
        series.viewMax = static_cast<float>(center + 0.5 * newSpan);
    }

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0F)) {
        const float plotHeight = ImPlot::GetPlotSize().y;
        if (plotHeight > 1.0F) {
            const double delta = static_cast<double>(io.MouseDelta.y) * currentSpan /
                                 static_cast<double>(plotHeight);
            series.viewMin = static_cast<float>(static_cast<double>(series.viewMin) + delta);
            series.viewMax = static_cast<float>(static_cast<double>(series.viewMax) + delta);
        }
    }
}
} // namespace

void Chart::draw(Dataset& dataset,
                 std::size_t& activeSeriesIndex,
                 ActiveHighlightMode activeHighlightMode,
                 double& xMin,
                 double& xMax,
                 bool showNativeLegend,
                 bool showCrosshair,
                 ChartMouseState& mouseState,
                 ChartLayoutState& layoutState,
                 CursorModel& cursorModel) {
    mouseState = {};
    layoutState = {};

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

    const ImVec2 size = ImGui::GetContentRegionAvail();
    const float yGutterWidth = semanticYAxisGutterWidth();
    const ImVec2 plotWidgetSize(std::max(1.0F, size.x - yGutterWidth), size.y);

    ImPlotFlags plotFlags =
        ImPlotFlags_NoTitle |
        ImPlotFlags_NoMenus |
        ImPlotFlags_NoBoxSelect;
    if (!showNativeLegend) {
        plotFlags |= ImPlotFlags_NoLegend;
    }

    Series* activeSeries = nullptr;
    if (activeSeriesIndex < dataset.series.size() && dataset.series[activeSeriesIndex].visible) {
        activeSeries = &dataset.series[activeSeriesIndex];
    }

    YTickData activeTicks;
    ImVec4 activeColor(1.0F, 1.0F, 1.0F, 1.0F);
    if (activeSeries != nullptr) {
        activeTicks = makeYTicks(*activeSeries, plotWidgetSize.y);
        activeColor = SeriesStyle::color(activeSeriesIndex);
    }

    const XTickData xTicks = makeXTicks(xMin, xMax, plotWidgetSize.x);

    ImPlotInputMap& inputMap = ImPlot::GetInputMap();
    inputMap.OverrideMod = ImGui::GetIO().KeyMods;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + yGutterWidth);

    if (ImPlot::BeginPlot("##MainChart", plotWidgetSize, plotFlags)) {
        constexpr ImPlotAxisFlags xFlags = ImPlotAxisFlags_NoLabel;
        ImPlot::SetupAxis(ImAxis_X1, nullptr, xFlags);
        ImPlot::SetupAxisLinks(ImAxis_X1, &xMin, &xMax);
        if (!xTicks.positions.empty()) {
            ImPlot::SetupAxisTicks(ImAxis_X1,
                                   xTicks.positions.data(),
                                   static_cast<int>(xTicks.positions.size()),
                                   xTicks.labelPointers.data(),
                                   false);
        }

        constexpr ImPlotAxisFlags yFlags =
            ImPlotAxisFlags_NoDecorations |
            ImPlotAxisFlags_NoGridLines |
            ImPlotAxisFlags_Lock;
        ImPlot::SetupAxis(ImAxis_Y1, nullptr, yFlags);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 1.0, ImPlotCond_Always);

        if (showNativeLegend) {
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_NoButtons);
        }

        const ImVec2 plotPos = ImPlot::GetPlotPos();
        const ImVec2 plotSize = ImPlot::GetPlotSize();
        layoutState.plotX = plotPos.x;
        layoutState.plotY = plotPos.y;
        layoutState.plotWidth = plotSize.x;
        layoutState.plotHeight = plotSize.y;

        updateMouseState(dataset, mouseState);
        handleSeriesSelection(dataset, activeSeriesIndex);

        drawHorizontalGrid(activeSeries,
                           activeSeries != nullptr ? &activeTicks : nullptr,
                           activeColor);

        for (std::size_t i = 0; i < dataset.series.size(); ++i) {
            if (i == activeSeriesIndex) {
                continue;
            }
            plotSeries(dataset, dataset.series[i], i, 1.0F);
        }

        if (activeSeriesIndex < dataset.series.size()) {
            const Series& highlightedSeries = dataset.series[activeSeriesIndex];
            plotActiveHighlight(dataset,
                                highlightedSeries,
                                activeSeriesIndex,
                                activeHighlightMode);
            plotSeries(dataset,
                       highlightedSeries,
                       activeSeriesIndex,
                       kActiveLineWeightPixels);
        }

        if (activeSeries != nullptr) {
            ImDrawList* drawList = ImPlot::GetPlotDrawList();
            drawList->AddText(ImVec2(plotPos.x + 6.0F, plotPos.y + 6.0F),
                              ImGui::ColorConvertFloat4ToU32(activeColor),
                              activeSeries->name.c_str());
        }

        const bool cursorOwnsLmb = CursorOverlay::draw(dataset, cursorModel);

        if (activeSeries != nullptr && !cursorOwnsLmb) {
            handleActiveYInput(*activeSeries);
        }

        if (showCrosshair) {
            drawCrosshair(mouseState);
        }

        ImPlot::EndPlot();
    }

    drawSemanticYAxis(layoutState,
                      activeSeries,
                      activeSeries != nullptr ? &activeTicks : nullptr,
                      activeColor);

    ImGui::End();
}
