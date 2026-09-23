#include "Chart.h"
#include "Dataset.h"
#include "TestEngine.h"

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
        {0.50F, 0.50F},
        {0.75F, 0.75F},
        {0.75F, 0.50F},
        {0.50F, 0.75F},
        {0.25F, 0.75F},
        {0.75F, 0.25F},
        {0.25F, 0.50F},
        {0.50F, 0.25F},
        {0.25F, 0.25F},
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

void resetState(ImGuiTestContext* ctx, bool keepActive, bool showOverlays) {
    if (gAccess.dataset == nullptr || gAccess.xMin == nullptr || gAccess.xMax == nullptr ||
        gAccess.activeSeriesIndex == nullptr) {
        return;
    }

    *gAccess.xMin = 0.0;
    *gAccess.xMax = gAccess.dataset->endTimeSeconds();
    for (Series& series : gAccess.dataset->series) {
        series.visible = true;
        series.fitViewToData();
    }

    *gAccess.activeSeriesIndex =
        keepActive && !gAccess.dataset->series.empty()
            ? 0
            : std::numeric_limits<std::size_t>::max();

    if (gAccess.showCustomLegend != nullptr) {
        *gAccess.showCustomLegend = showOverlays;
    }
    if (gAccess.showValues != nullptr) {
        *gAccess.showValues = showOverlays;
    }

    ctx->Yield(2);
}
} // namespace

void RegisterNavigationTests(ImGuiTestEngine* engine, const GuiTestAccess& access) {
    gAccess = access;

    ImGuiTest* test = IM_REGISTER_TEST(engine, "navigation", "plain_wheel_x_only");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        IM_CHECK(gAccess.dataset != nullptr);
        IM_CHECK(gAccess.xMin != nullptr);
        IM_CHECK(gAccess.xMax != nullptr);
        IM_CHECK(gAccess.activeSeriesIndex != nullptr);
        IM_CHECK(!gAccess.dataset->series.empty());

        resetState(ctx, true, false);
        IM_CHECK(moveToPlot(ctx));

        Series& active = gAccess.dataset->series[0];
        const double xMinBefore = *gAccess.xMin;
        const double xMaxBefore = *gAccess.xMax;
        const double xSpanBefore = xMaxBefore - xMinBefore;
        const float yMinBefore = active.viewMin;
        const float yMaxBefore = active.viewMax;

        ctx->MouseWheelY(1.0F);
        ctx->Yield(2);

        const double xSpanAfter = *gAccess.xMax - *gAccess.xMin;
        IM_CHECK(!nearlyEqual(xSpanAfter, xSpanBefore, 1.0e-4));
        IM_CHECK(nearlyEqual(active.viewMin, yMinBefore, 1.0e-5));
        IM_CHECK(nearlyEqual(active.viewMax, yMaxBefore, 1.0e-5));

        resetState(ctx, false, true);
    };

    test = IM_REGISTER_TEST(engine, "navigation", "plain_drag_x_only");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        IM_CHECK(gAccess.dataset != nullptr);
        IM_CHECK(gAccess.xMin != nullptr);
        IM_CHECK(gAccess.xMax != nullptr);
        IM_CHECK(gAccess.activeSeriesIndex != nullptr);
        IM_CHECK(!gAccess.dataset->series.empty());

        resetState(ctx, true, false);
        const double dataEnd = gAccess.dataset->endTimeSeconds();
        *gAccess.xMin = dataEnd * 0.25;
        *gAccess.xMax = dataEnd * 0.75;
        ctx->Yield(2);
        IM_CHECK(moveToPlot(ctx));

        Series& active = gAccess.dataset->series[0];
        const double xMinBefore = *gAccess.xMin;
        const double xMaxBefore = *gAccess.xMax;
        const double xSpanBefore = xMaxBefore - xMinBefore;
        const float yMinBefore = active.viewMin;
        const float yMaxBefore = active.viewMax;

        ctx->MouseDragWithDelta(ImVec2(120.0F, 0.0F), ImGuiMouseButton_Left);
        ctx->Yield(2);

        const double xSpanAfter = *gAccess.xMax - *gAccess.xMin;
        IM_CHECK(!nearlyEqual(*gAccess.xMin, xMinBefore, 1.0e-4));
        IM_CHECK(!nearlyEqual(*gAccess.xMax, xMaxBefore, 1.0e-4));
        IM_CHECK(nearlyEqual(xSpanAfter, xSpanBefore, 1.0e-3));
        IM_CHECK(nearlyEqual(active.viewMin, yMinBefore, 1.0e-5));
        IM_CHECK(nearlyEqual(active.viewMax, yMaxBefore, 1.0e-5));

        resetState(ctx, false, true);
    };

    test = IM_REGISTER_TEST(engine, "navigation", "alt_wheel_y_only");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        IM_CHECK(gAccess.dataset != nullptr);
        IM_CHECK(gAccess.xMin != nullptr);
        IM_CHECK(gAccess.xMax != nullptr);
        IM_CHECK(gAccess.activeSeriesIndex != nullptr);
        IM_CHECK(!gAccess.dataset->series.empty());

        resetState(ctx, true, false);
        IM_CHECK(moveToPlot(ctx));

        Series& active = gAccess.dataset->series[0];
        const double xMinBefore = *gAccess.xMin;
        const double xMaxBefore = *gAccess.xMax;
        const double yCenterBefore = 0.5 * (active.viewMin + active.viewMax);
        const double ySpanBefore = static_cast<double>(active.viewMax - active.viewMin);

        ctx->KeyDown(ImGuiMod_Alt);
        ctx->Yield();
        ctx->MouseWheelY(1.0F);
        ctx->Yield(2);
        ctx->KeyUp(ImGuiMod_Alt);
        ctx->Yield();

        const double yCenterAfter = 0.5 * (active.viewMin + active.viewMax);
        const double ySpanAfter = static_cast<double>(active.viewMax - active.viewMin);
        IM_CHECK(nearlyEqual(*gAccess.xMin, xMinBefore, 1.0e-5));
        IM_CHECK(nearlyEqual(*gAccess.xMax, xMaxBefore, 1.0e-5));
        IM_CHECK(ySpanAfter < ySpanBefore);
        IM_CHECK(nearlyEqual(ySpanAfter, ySpanBefore * 0.85, 1.0e-3));
        IM_CHECK(nearlyEqual(yCenterAfter, yCenterBefore, 1.0e-4));

        resetState(ctx, false, true);
    };

    test = IM_REGISTER_TEST(engine, "navigation", "alt_drag_y_only");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        IM_CHECK(gAccess.dataset != nullptr);
        IM_CHECK(gAccess.xMin != nullptr);
        IM_CHECK(gAccess.xMax != nullptr);
        IM_CHECK(gAccess.activeSeriesIndex != nullptr);
        IM_CHECK(!gAccess.dataset->series.empty());

        resetState(ctx, true, false);
        const double dataEnd = gAccess.dataset->endTimeSeconds();
        *gAccess.xMin = dataEnd * 0.25;
        *gAccess.xMax = dataEnd * 0.75;
        ctx->Yield(2);
        IM_CHECK(moveToPlot(ctx));

        Series& active = gAccess.dataset->series[0];
        const double xMinBefore = *gAccess.xMin;
        const double xMaxBefore = *gAccess.xMax;
        const double yCenterBefore = 0.5 * (active.viewMin + active.viewMax);
        const double ySpanBefore = static_cast<double>(active.viewMax - active.viewMin);

        ctx->KeyDown(ImGuiMod_Alt);
        ctx->Yield();
        ctx->MouseDragWithDelta(ImVec2(80.0F, 60.0F), ImGuiMouseButton_Left);
        ctx->Yield(2);
        ctx->KeyUp(ImGuiMod_Alt);
        ctx->Yield();

        const double yCenterAfter = 0.5 * (active.viewMin + active.viewMax);
        const double ySpanAfter = static_cast<double>(active.viewMax - active.viewMin);
        IM_CHECK(nearlyEqual(*gAccess.xMin, xMinBefore, 1.0e-5));
        IM_CHECK(nearlyEqual(*gAccess.xMax, xMaxBefore, 1.0e-5));
        IM_CHECK(!nearlyEqual(yCenterAfter, yCenterBefore, 1.0e-4));
        IM_CHECK(nearlyEqual(ySpanAfter, ySpanBefore, 1.0e-3));

        resetState(ctx, false, true);
    };
}
