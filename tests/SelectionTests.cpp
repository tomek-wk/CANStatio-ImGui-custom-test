#include "Chart.h"
#include "Dataset.h"
#include "TestEngine.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

#include <imgui_internal.h>
#include <imgui_test_engine/imgui_te_context.h>

namespace {
GuiTestAccess gAccess;

struct PlotRect {
    float left = 0.0F;
    float top = 0.0F;
    float right = 0.0F;
    float bottom = 0.0F;
};

struct EmptyPoint {
    std::size_t sampleIndex = 0;
    double normalizedY = 0.5;
    double separation = 0.0;
};

bool probePlot(ImGuiTestContext* ctx, const ImVec2& pos) {
    ctx->MouseTeleportToPos(pos);
    ctx->Yield();
    return gAccess.chartMouseState != nullptr && gAccess.chartMouseState->plotHovered;
}

bool findPointFromCandidates(ImGuiTestContext* ctx,
                             ImGuiWindow* chartHost,
                             const float candidates[][2],
                             std::size_t candidateCount,
                             ImVec2& point) {
    const ImVec2 min = chartHost->InnerRect.Min;
    const ImVec2 max = chartHost->InnerRect.Max;
    const float width = max.x - min.x;
    const float height = max.y - min.y;

    for (std::size_t i = 0; i < candidateCount; ++i) {
        point = ImVec2(min.x + width * candidates[i][0], min.y + height * candidates[i][1]);
        if (probePlot(ctx, point)) {
            return true;
        }
    }
    return false;
}

float findPlotEdge(ImGuiTestContext* ctx,
                   const ImVec2& interior,
                   bool horizontal,
                   int direction,
                   float limit) {
    const float insideCoordinate = horizontal ? interior.x : interior.y;
    float lastInside = insideCoordinate;
    float firstOutside = limit;
    const float step = 24.0F * static_cast<float>(direction);

    for (float coordinate = insideCoordinate + step;
         direction < 0 ? coordinate > limit : coordinate < limit;
         coordinate += step) {
        ImVec2 probe = interior;
        if (horizontal) {
            probe.x = coordinate;
        } else {
            probe.y = coordinate;
        }

        if (probePlot(ctx, probe)) {
            lastInside = coordinate;
        } else {
            firstOutside = coordinate;
            break;
        }
    }

    ImVec2 limitProbe = interior;
    if (horizontal) {
        limitProbe.x = limit;
    } else {
        limitProbe.y = limit;
    }
    if (probePlot(ctx, limitProbe)) {
        return limit;
    }

    float inside = lastInside;
    float outside = firstOutside;
    for (int iteration = 0; iteration < 10; ++iteration) {
        const float middle = 0.5F * (inside + outside);
        ImVec2 probe = interior;
        if (horizontal) {
            probe.x = middle;
        } else {
            probe.y = middle;
        }

        if (probePlot(ctx, probe)) {
            inside = middle;
        } else {
            outside = middle;
        }
    }

    return inside;
}

bool findPlotRect(ImGuiTestContext* ctx, PlotRect& rect) {
    ImGuiWindow* chartHost = ImGui::FindWindowByName("ChartHost");
    if (chartHost == nullptr || gAccess.chartMouseState == nullptr) {
        return false;
    }

    ctx->MouseSetViewport(chartHost);

    // Floating windows can cover parts of ChartHost. Probe horizontal edges
    // low in the plot (normally below Test Controls), and vertical edges
    // through the middle. This avoids mistaking an overlay edge for PlotRect.
    constexpr float horizontalCandidates[][2] = {
        {0.50F, 0.80F},
        {0.25F, 0.80F},
        {0.75F, 0.80F},
        {0.50F, 0.70F},
        {0.25F, 0.70F},
        {0.75F, 0.70F},
    };
    constexpr float verticalCandidates[][2] = {
        {0.50F, 0.50F},
        {0.50F, 0.75F},
        {0.50F, 0.25F},
        {0.40F, 0.50F},
        {0.60F, 0.50F},
    };

    ImVec2 horizontalInterior;
    ImVec2 verticalInterior;
    if (!findPointFromCandidates(ctx,
                                 chartHost,
                                 horizontalCandidates,
                                 std::size(horizontalCandidates),
                                 horizontalInterior) ||
        !findPointFromCandidates(ctx,
                                 chartHost,
                                 verticalCandidates,
                                 std::size(verticalCandidates),
                                 verticalInterior)) {
        return false;
    }

    rect.left = findPlotEdge(ctx,
                             horizontalInterior,
                             true,
                             -1,
                             chartHost->InnerRect.Min.x);
    rect.right = findPlotEdge(ctx,
                              horizontalInterior,
                              true,
                              +1,
                              chartHost->InnerRect.Max.x);
    rect.top = findPlotEdge(ctx,
                            verticalInterior,
                            false,
                            -1,
                            chartHost->InnerRect.Min.y);
    rect.bottom = findPlotEdge(ctx,
                               verticalInterior,
                               false,
                               +1,
                               chartHost->InnerRect.Max.y);

    return rect.right - rect.left > 100.0F && rect.bottom - rect.top > 100.0F;
}

double normalizedValue(const Series& series, std::size_t sampleIndex) {
    const double span = static_cast<double>(series.viewMax - series.viewMin);
    if (!(span > 0.0) || sampleIndex >= series.values.size()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (static_cast<double>(series.values[sampleIndex]) - series.viewMin) / span;
}

std::size_t chooseSeparatedSample(const Dataset& dataset, std::size_t targetSeriesIndex) {
    const std::size_t sampleCount = dataset.sampleCount;
    if (targetSeriesIndex >= dataset.series.size() || sampleCount < 10) {
        return 0;
    }

    const std::size_t begin = sampleCount / 10;
    const std::size_t end = sampleCount - sampleCount / 10;
    std::size_t bestIndex = begin;
    double bestSeparation = -1.0;

    for (std::size_t i = begin; i < end; ++i) {
        const double targetY = normalizedValue(dataset.series[targetSeriesIndex], i);
        if (!std::isfinite(targetY) || targetY < 0.02 || targetY > 0.98) {
            continue;
        }

        double separation = 2.0;
        for (std::size_t j = 0; j < dataset.series.size(); ++j) {
            if (j == targetSeriesIndex || !dataset.series[j].visible) {
                continue;
            }
            const double otherY = normalizedValue(dataset.series[j], i);
            if (std::isfinite(otherY)) {
                separation = std::min(separation, std::abs(targetY - otherY));
            }
        }

        if (separation > bestSeparation) {
            bestSeparation = separation;
            bestIndex = i;
        }
    }

    return bestIndex;
}

EmptyPoint chooseEmptyPoint(const Dataset& dataset) {
    EmptyPoint best;
    const std::size_t sampleCount = dataset.sampleCount;
    if (sampleCount < 10) {
        return best;
    }

    const std::size_t begin = sampleCount / 10;
    const std::size_t end = sampleCount - sampleCount / 10;

    for (std::size_t i = begin; i < end; i += 5) {
        for (int yStep = 1; yStep < 20; ++yStep) {
            const double candidateY = static_cast<double>(yStep) / 20.0;
            double separation = 2.0;

            for (const Series& series : dataset.series) {
                if (!series.visible) {
                    continue;
                }
                const double seriesY = normalizedValue(series, i);
                if (std::isfinite(seriesY)) {
                    separation = std::min(separation, std::abs(candidateY - seriesY));
                }
            }

            if (separation > best.separation) {
                best = EmptyPoint{.sampleIndex = i,
                                  .normalizedY = candidateY,
                                  .separation = separation};
            }
        }
    }

    return best;
}

ImVec2 pointForSample(const PlotRect& rect,
                      const Dataset& dataset,
                      std::size_t sampleIndex,
                      double normalizedY,
                      double xMin,
                      double xMax) {
    const double time = static_cast<double>(sampleIndex) * dataset.dtSeconds;
    const double xFraction = (time - xMin) / (xMax - xMin);
    const float width = rect.right - rect.left;
    const float height = rect.bottom - rect.top;
    return ImVec2(rect.left + static_cast<float>(xFraction) * width,
                  rect.bottom - static_cast<float>(normalizedY) * height);
}

void resetState(ImGuiTestContext* ctx) {
    if (gAccess.dataset == nullptr || gAccess.xMin == nullptr || gAccess.xMax == nullptr ||
        gAccess.activeSeriesIndex == nullptr) {
        return;
    }

    *gAccess.xMin = 0.0;
    *gAccess.xMax = gAccess.dataset->endTimeSeconds();
    *gAccess.activeSeriesIndex = std::numeric_limits<std::size_t>::max();

    for (Series& series : gAccess.dataset->series) {
        series.visible = true;
        series.fitViewToData();
    }

    if (gAccess.showNativeLegend != nullptr) {
        *gAccess.showNativeLegend = false;
    }
    if (gAccess.showCustomLegend != nullptr) {
        *gAccess.showCustomLegend = false;
    }
    if (gAccess.showValues != nullptr) {
        *gAccess.showValues = false;
    }
    if (gAccess.showCrosshair != nullptr) {
        *gAccess.showCrosshair = false;
    }
    ctx->Yield(2);
}

void restoreState(ImGuiTestContext* ctx) {
    if (gAccess.dataset != nullptr) {
        for (Series& series : gAccess.dataset->series) {
            series.visible = true;
        }
    }
    if (gAccess.activeSeriesIndex != nullptr) {
        *gAccess.activeSeriesIndex = std::numeric_limits<std::size_t>::max();
    }
    if (gAccess.showNativeLegend != nullptr) {
        *gAccess.showNativeLegend = true;
    }
    if (gAccess.showCustomLegend != nullptr) {
        *gAccess.showCustomLegend = true;
    }
    if (gAccess.showValues != nullptr) {
        *gAccess.showValues = true;
    }
    if (gAccess.showCrosshair != nullptr) {
        *gAccess.showCrosshair = true;
    }
    ctx->Yield(2);
}
} // namespace

void RegisterSelectionTests(ImGuiTestEngine* engine, const GuiTestAccess& access) {
    gAccess = access;

    ImGuiTest* test = IM_REGISTER_TEST(engine, "selection", "rmb_select_and_toggle");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        IM_CHECK(gAccess.dataset != nullptr);
        IM_CHECK(gAccess.dataset->series.size() >= 3);
        IM_CHECK(gAccess.activeSeriesIndex != nullptr);
        IM_CHECK(gAccess.xMin != nullptr);
        IM_CHECK(gAccess.xMax != nullptr);

        resetState(ctx);

        const std::size_t sampleIndex = chooseSeparatedSample(*gAccess.dataset, 0);
        const double targetY = normalizedValue(gAccess.dataset->series[0], sampleIndex);
        IM_CHECK(std::isfinite(targetY));

        PlotRect rect;
        IM_CHECK(findPlotRect(ctx, rect));
        const float plotHeight = rect.bottom - rect.top;

        double minimumSeparation = 2.0;
        for (std::size_t j = 1; j < gAccess.dataset->series.size(); ++j) {
            minimumSeparation = std::min(
                minimumSeparation,
                std::abs(targetY - normalizedValue(gAccess.dataset->series[j], sampleIndex)));
        }
        IM_CHECK(minimumSeparation * plotHeight > 12.0);

        ImVec2 target = pointForSample(rect,
                                       *gAccess.dataset,
                                       sampleIndex,
                                       targetY,
                                       *gAccess.xMin,
                                       *gAccess.xMax);
        IM_CHECK(probePlot(ctx, target));
        ctx->MouseClick(ImGuiMouseButton_Right);
        ctx->Yield(2);
        ctx->LogInfo("RMB line target selected series index %zu", *gAccess.activeSeriesIndex);
        IM_CHECK(*gAccess.activeSeriesIndex == 0);

        IM_CHECK(findPlotRect(ctx, rect));
        target = pointForSample(rect,
                                *gAccess.dataset,
                                sampleIndex,
                                normalizedValue(gAccess.dataset->series[0], sampleIndex),
                                *gAccess.xMin,
                                *gAccess.xMax);
        IM_CHECK(probePlot(ctx, target));
        ctx->MouseClick(ImGuiMouseButton_Right);
        ctx->Yield(2);
        IM_CHECK(*gAccess.activeSeriesIndex == std::numeric_limits<std::size_t>::max());

        restoreState(ctx);
    };

    test = IM_REGISTER_TEST(engine, "selection", "rmb_hidden_and_empty");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        IM_CHECK(gAccess.dataset != nullptr);
        IM_CHECK(gAccess.dataset->series.size() >= 3);
        IM_CHECK(gAccess.activeSeriesIndex != nullptr);
        IM_CHECK(gAccess.xMin != nullptr);
        IM_CHECK(gAccess.xMax != nullptr);

        resetState(ctx);

        gAccess.dataset->series[0].visible = false;
        ctx->Yield(2);
        const std::size_t hiddenSample = chooseSeparatedSample(*gAccess.dataset, 0);
        const double hiddenY = normalizedValue(gAccess.dataset->series[0], hiddenSample);
        IM_CHECK(std::isfinite(hiddenY));

        PlotRect rect;
        IM_CHECK(findPlotRect(ctx, rect));
        const float hiddenPlotHeight = rect.bottom - rect.top;
        double hiddenSeparation = 2.0;
        for (std::size_t j = 1; j < gAccess.dataset->series.size(); ++j) {
            if (!gAccess.dataset->series[j].visible) {
                continue;
            }
            hiddenSeparation = std::min(
                hiddenSeparation,
                std::abs(hiddenY - normalizedValue(gAccess.dataset->series[j], hiddenSample)));
        }
        IM_CHECK(hiddenSeparation * hiddenPlotHeight > 12.0);

        const ImVec2 hiddenPoint = pointForSample(rect,
                                                  *gAccess.dataset,
                                                  hiddenSample,
                                                  hiddenY,
                                                  *gAccess.xMin,
                                                  *gAccess.xMax);
        IM_CHECK(probePlot(ctx, hiddenPoint));
        ctx->MouseClick(ImGuiMouseButton_Right);
        ctx->Yield(2);
        IM_CHECK(*gAccess.activeSeriesIndex == std::numeric_limits<std::size_t>::max());

        gAccess.dataset->series[0].visible = true;
        *gAccess.activeSeriesIndex = 1;
        ctx->Yield(2);

        const EmptyPoint empty = chooseEmptyPoint(*gAccess.dataset);
        IM_CHECK(empty.separation > 0.0);
        IM_CHECK(findPlotRect(ctx, rect));
        const float plotHeight = rect.bottom - rect.top;
        IM_CHECK(empty.separation * plotHeight > 12.0);

        const ImVec2 emptyScreen = pointForSample(rect,
                                                  *gAccess.dataset,
                                                  empty.sampleIndex,
                                                  empty.normalizedY,
                                                  *gAccess.xMin,
                                                  *gAccess.xMax);
        IM_CHECK(probePlot(ctx, emptyScreen));
        ctx->MouseClick(ImGuiMouseButton_Right);
        ctx->Yield(2);
        IM_CHECK(*gAccess.activeSeriesIndex == std::numeric_limits<std::size_t>::max());

        restoreState(ctx);
    };
}
