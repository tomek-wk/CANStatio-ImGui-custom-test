#include "Chart.h"

#include "Dataset.h"
#include "SeriesStyle.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

#include <imgui.h>

namespace {
constexpr float kLeftMargin = 72.0F;
constexpr float kRightMargin = 16.0F;
constexpr float kTopMargin = 16.0F;
constexpr float kBottomMargin = 40.0F;
constexpr float kInactiveLineWeight = 1.0F;
constexpr float kActiveLineWeight = 1.5F;
constexpr float kHaloWeight = 7.0F;
constexpr float kHaloAlpha = 0.22F;
constexpr float kOutlineWeight = 5.0F;
constexpr float kOutlineAlpha = 0.65F;

bool validRange(double minimum, double maximum) {
    return std::isfinite(minimum) && std::isfinite(maximum) && maximum > minimum;
}

ImVec2 dataToScreen(double timeSeconds,
                    double value,
                    double xMin,
                    double xMax,
                    double yMin,
                    double yMax,
                    const ImVec2& plotMin,
                    const ImVec2& plotMax) {
    const double xFraction = (timeSeconds - xMin) / (xMax - xMin);
    const double yFraction = (value - yMin) / (yMax - yMin);

    return ImVec2(
        plotMin.x + static_cast<float>(xFraction) * (plotMax.x - plotMin.x),
        plotMax.y - static_cast<float>(yFraction) * (plotMax.y - plotMin.y));
}

std::vector<ImVec2> makeSeriesPoints(const Dataset& dataset,
                                     const Series& series,
                                     double xMin,
                                     double xMax,
                                     const ImVec2& plotMin,
                                     const ImVec2& plotMax) {
    std::vector<ImVec2> points;

    if (!series.visible || series.values.size() < 2 ||
        !validRange(xMin, xMax) ||
        !validRange(series.viewMin, series.viewMax) ||
        !(dataset.dtSeconds > 0.0)) {
        return points;
    }

    const long long maxIndex = static_cast<long long>(series.values.size()) - 1;
    long long firstIndex = static_cast<long long>(std::floor(xMin / dataset.dtSeconds)) - 1;
    long long lastIndex = static_cast<long long>(std::ceil(xMax / dataset.dtSeconds)) + 1;

    firstIndex = std::clamp(firstIndex, 0LL, maxIndex);
    lastIndex = std::clamp(lastIndex, 0LL, maxIndex);
    if (lastIndex <= firstIndex) {
        return points;
    }

    points.reserve(static_cast<std::size_t>(lastIndex - firstIndex + 1));
    for (long long index = firstIndex; index <= lastIndex; ++index) {
        const std::size_t sampleIndex = static_cast<std::size_t>(index);
        const double timeSeconds = static_cast<double>(sampleIndex) * dataset.dtSeconds;
        const double value = static_cast<double>(series.values[sampleIndex]);
        points.push_back(dataToScreen(timeSeconds,
                                      value,
                                      xMin,
                                      xMax,
                                      series.viewMin,
                                      series.viewMax,
                                      plotMin,
                                      plotMax));
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

void drawSeries(ImDrawList* drawList,
                const Dataset& dataset,
                const Series& series,
                std::size_t seriesIndex,
                double xMin,
                double xMax,
                const ImVec2& plotMin,
                const ImVec2& plotMax,
                float weight) {
    const std::vector<ImVec2> points =
        makeSeriesPoints(dataset, series, xMin, xMax, plotMin, plotMax);
    drawPolyline(drawList, points, SeriesStyle::color(seriesIndex), weight);
}

void drawActiveSeries(ImDrawList* drawList,
                      const Dataset& dataset,
                      const Series& series,
                      std::size_t seriesIndex,
                      ActiveHighlightMode mode,
                      double xMin,
                      double xMax,
                      const ImVec2& plotMin,
                      const ImVec2& plotMax) {
    const std::vector<ImVec2> points =
        makeSeriesPoints(dataset, series, xMin, xMax, plotMin, plotMax);
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

void updateMouseState(const Dataset& dataset,
                      double xMin,
                      double xMax,
                      const ImVec2& plotMin,
                      const ImVec2& plotMax,
                      bool plotHovered,
                      ChartMouseState& mouseState) {
    mouseState = {};
    mouseState.plotHovered = plotHovered;
    if (!plotHovered || !validRange(xMin, xMax) || plotMax.x <= plotMin.x) {
        return;
    }

    const float mouseX = ImGui::GetMousePos().x;
    const double xFraction = static_cast<double>((mouseX - plotMin.x) / (plotMax.x - plotMin.x));
    mouseState.timeSeconds = xMin + xFraction * (xMax - xMin);
    mouseState.withinDataset =
        dataset.sampleCount > 0 &&
        mouseState.timeSeconds >= 0.0 &&
        mouseState.timeSeconds <= dataset.endTimeSeconds();
}
} // namespace

void Chart::draw(Dataset& dataset,
                 std::size_t& activeSeriesIndex,
                 ActiveHighlightMode activeHighlightMode,
                 double& xMin,
                 double& xMax,
                 bool showCrosshair,
                 ChartMouseState& mouseState,
                 ChartLayoutState& layoutState) {
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

    const ImVec2 canvasMin = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    const float plotWidth = std::max(1.0F, canvasSize.x - kLeftMargin - kRightMargin);
    const float plotHeight = std::max(1.0F, canvasSize.y - kTopMargin - kBottomMargin);
    const ImVec2 plotMin(canvasMin.x + kLeftMargin, canvasMin.y + kTopMargin);
    const ImVec2 plotMax(plotMin.x + plotWidth, plotMin.y + plotHeight);

    layoutState.plotX = plotMin.x;
    layoutState.plotY = plotMin.y;
    layoutState.plotWidth = plotWidth;
    layoutState.plotHeight = plotHeight;

    ImGui::SetCursorScreenPos(plotMin);
    ImGui::InvisibleButton("##CustomPlotInput",
                           ImVec2(plotWidth, plotHeight),
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    const bool plotHovered = ImGui::IsItemHovered();

    updateMouseState(dataset, xMin, xMax, plotMin, plotMax, plotHovered, mouseState);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(plotMin, plotMax, ImGui::GetColorU32(ImGuiCol_FrameBg));

    drawList->PushClipRect(plotMin, plotMax, true);

    for (std::size_t i = 0; i < dataset.series.size(); ++i) {
        if (i == activeSeriesIndex) {
            continue;
        }
        drawSeries(drawList,
                   dataset,
                   dataset.series[i],
                   i,
                   xMin,
                   xMax,
                   plotMin,
                   plotMax,
                   kInactiveLineWeight);
    }

    if (activeSeriesIndex < dataset.series.size()) {
        const Series& activeSeries = dataset.series[activeSeriesIndex];
        if (activeSeries.visible) {
            drawActiveSeries(drawList,
                             dataset,
                             activeSeries,
                             activeSeriesIndex,
                             activeHighlightMode,
                             xMin,
                             xMax,
                             plotMin,
                             plotMax);
        }
    }

    if (showCrosshair && mouseState.plotHovered) {
        const float mouseX = ImGui::GetMousePos().x;
        ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        color.w *= 0.45F;
        drawList->AddLine(ImVec2(mouseX, plotMin.y),
                          ImVec2(mouseX, plotMax.y),
                          ImGui::ColorConvertFloat4ToU32(color),
                          1.0F);
    }

    drawList->PopClipRect();

    drawList->AddRect(plotMin,
                      plotMax,
                      ImGui::GetColorU32(ImGuiCol_Border),
                      0.0F,
                      ImDrawFlags_None,
                      1.0F);

    if (activeSeriesIndex < dataset.series.size() && dataset.series[activeSeriesIndex].visible) {
        const Series& activeSeries = dataset.series[activeSeriesIndex];
        drawList->AddText(ImVec2(plotMin.x + 6.0F, plotMin.y + 6.0F),
                          ImGui::ColorConvertFloat4ToU32(SeriesStyle::color(activeSeriesIndex)),
                          activeSeries.name.c_str());
    }

    ImGui::End();
}
