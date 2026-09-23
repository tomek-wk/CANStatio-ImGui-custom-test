#include "Chart.h"
#include "Dataset.h"
#include "TestEngine.h"
#include "ValuesModel.h"

#include <cmath>
#include <limits>

#include <imgui_internal.h>
#include <imgui_test_engine/imgui_te_context.h>

namespace {
GuiTestAccess gAccess;

bool nearlyEqual(double lhs, double rhs, double epsilon = 1.0e-6) {
    return std::abs(lhs - rhs) <= epsilon;
}

bool moveToPlot(ImGuiTestContext* ctx) {
    ImGuiWindow* chartHost = ImGui::FindWindowByName("ChartHost");
    if (chartHost == nullptr || gAccess.chartMouseState == nullptr) {
        return false;
    }

    ctx->MouseSetViewport(chartHost);

    constexpr float candidates[][2] = {
        {0.75F, 0.75F},
        {0.75F, 0.50F},
        {0.50F, 0.75F},
        {0.50F, 0.50F},
        {0.25F, 0.75F},
        {0.75F, 0.25F},
    };

    const ImVec2 min = chartHost->InnerRect.Min;
    const ImVec2 max = chartHost->InnerRect.Max;
    const ImVec2 size(max.x - min.x, max.y - min.y);

    for (const auto& candidate : candidates) {
        const ImVec2 pos(min.x + size.x * candidate[0], min.y + size.y * candidate[1]);
        ctx->MouseTeleportToPos(pos);
        ctx->Yield(2);
        if (gAccess.chartMouseState->plotHovered) {
            return true;
        }
    }

    return false;
}

void prepareValuesState(ImGuiTestContext* ctx) {
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

    if (gAccess.showValues != nullptr) {
        *gAccess.showValues = true;
    }
    if (gAccess.showCustomLegend != nullptr) {
        *gAccess.showCustomLegend = false;
    }
    if (gAccess.showCrosshair != nullptr) {
        *gAccess.showCrosshair = true;
    }

    ctx->Yield(2);
}

void restoreWindows(ImGuiTestContext* ctx) {
    if (gAccess.showValues != nullptr) {
        *gAccess.showValues = true;
    }
    if (gAccess.showCustomLegend != nullptr) {
        *gAccess.showCustomLegend = true;
    }
    if (gAccess.showCrosshair != nullptr) {
        *gAccess.showCrosshair = true;
    }
    if (gAccess.dataset != nullptr) {
        for (Series& series : gAccess.dataset->series) {
            series.visible = true;
        }
    }
    if (gAccess.xMin != nullptr && gAccess.xMax != nullptr && gAccess.dataset != nullptr) {
        *gAccess.xMin = 0.0;
        *gAccess.xMax = gAccess.dataset->endTimeSeconds();
    }
    ctx->Yield(2);
}
} // namespace

void RegisterValuesTests(ImGuiTestEngine* engine, const GuiTestAccess& access) {
    gAccess = access;

    ImGuiTest* test = IM_REGISTER_TEST(engine, "values", "model_visible_and_interpolated");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        IM_CHECK(gAccess.dataset != nullptr);
        IM_CHECK(gAccess.dataset->series.size() >= 3);
        IM_CHECK(gAccess.dataset->sampleCount > 12);

        prepareValuesState(ctx);

        const std::size_t lowerIndex = 10;
        ChartMouseState state;
        state.plotHovered = true;
        state.withinDataset = true;
        state.timeSeconds = (static_cast<double>(lowerIndex) + 0.5) * gAccess.dataset->dtSeconds;

        const ValuesSnapshot snapshot = makeValuesSnapshot(*gAccess.dataset, state);
        IM_CHECK(snapshot.plotHovered);
        IM_CHECK(snapshot.withinDataset);
        IM_CHECK(snapshot.rows.size() == 3);

        for (std::size_t rowIndex = 0; rowIndex < snapshot.rows.size(); ++rowIndex) {
            const ValuesRow& row = snapshot.rows[rowIndex];
            IM_CHECK(row.seriesIndex == rowIndex);
            const Series& series = gAccess.dataset->series[row.seriesIndex];
            const double expected =
                0.5 * (static_cast<double>(series.values[lowerIndex]) +
                       static_cast<double>(series.values[lowerIndex + 1]));
            IM_CHECK(nearlyEqual(row.value, expected, 1.0e-6));
        }

        gAccess.dataset->series[1].visible = false;
        const ValuesSnapshot hidden = makeValuesSnapshot(*gAccess.dataset, state);
        IM_CHECK(hidden.rows.size() == 2);
        IM_CHECK(hidden.rows[0].seriesIndex == 0);
        IM_CHECK(hidden.rows[1].seriesIndex == 2);

        state.withinDataset = false;
        const ValuesSnapshot outside = makeValuesSnapshot(*gAccess.dataset, state);
        IM_CHECK(outside.rows.empty());

        state.withinDataset = true;
        state.plotHovered = false;
        const ValuesSnapshot cleared = makeValuesSnapshot(*gAccess.dataset, state);
        IM_CHECK(cleared.rows.empty());

        restoreWindows(ctx);
    };

    test = IM_REGISTER_TEST(engine, "values", "mouse_state_and_crosshair_independent");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        IM_CHECK(gAccess.dataset != nullptr);
        IM_CHECK(gAccess.dataset->series.size() >= 3);
        IM_CHECK(gAccess.chartMouseState != nullptr);
        IM_CHECK(gAccess.showCrosshair != nullptr);
        IM_CHECK(gAccess.xMin != nullptr);
        IM_CHECK(gAccess.xMax != nullptr);

        prepareValuesState(ctx);
        IM_CHECK(moveToPlot(ctx));
        IM_CHECK(gAccess.chartMouseState->withinDataset);

        ValuesSnapshot snapshot =
            makeValuesSnapshot(*gAccess.dataset, *gAccess.chartMouseState);
        IM_CHECK(snapshot.rows.size() == 3);

        *gAccess.showCrosshair = false;
        ctx->Yield(2);
        IM_CHECK(!*gAccess.showCrosshair);
        IM_CHECK(gAccess.chartMouseState->plotHovered);
        IM_CHECK(gAccess.chartMouseState->withinDataset);

        snapshot = makeValuesSnapshot(*gAccess.dataset, *gAccess.chartMouseState);
        IM_CHECK(snapshot.rows.size() == 3);

        const double dataEnd = gAccess.dataset->endTimeSeconds();
        *gAccess.xMin = dataEnd * 1.20;
        *gAccess.xMax = dataEnd * 1.60;
        ctx->Yield(2);
        IM_CHECK(gAccess.chartMouseState->plotHovered);
        IM_CHECK(!gAccess.chartMouseState->withinDataset);

        snapshot = makeValuesSnapshot(*gAccess.dataset, *gAccess.chartMouseState);
        IM_CHECK(snapshot.rows.empty());

        ctx->SetRef("Test Controls");
        ctx->MouseMove("Fit X");
        ctx->Yield(2);
        IM_CHECK(!gAccess.chartMouseState->plotHovered);

        snapshot = makeValuesSnapshot(*gAccess.dataset, *gAccess.chartMouseState);
        IM_CHECK(snapshot.rows.empty());

        restoreWindows(ctx);
    };
}
