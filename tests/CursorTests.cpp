#include "Chart.h"
#include "CursorModel.h"
#include "Dataset.h"
#include "TestEngine.h"

#include <cmath>
#include <limits>

#include <imgui_internal.h>
#include <imgui_test_engine/imgui_te_context.h>

namespace {
GuiTestAccess gAccess;

bool nearlyEqual(double lhs, double rhs, double epsilon = 1.0e-5) {
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
        {0.70F, 0.65F},
        {0.70F, 0.35F},
        {0.30F, 0.65F},
        {0.30F, 0.35F},
    };

    const ImVec2 min = chartHost->InnerRect.Min;
    const ImVec2 max = chartHost->InnerRect.Max;
    const ImVec2 size(max.x - min.x, max.y - min.y);

    for (const auto& candidate : candidates) {
        ctx->MouseTeleportToPos(ImVec2(min.x + size.x * candidate[0],
                                       min.y + size.y * candidate[1]));
        ctx->Yield(2);
        if (gAccess.chartMouseState->plotHovered) {
            return true;
        }
    }

    return false;
}

void resetState(ImGuiTestContext* ctx, bool activeSeries) {
    if (gAccess.dataset == nullptr || gAccess.cursorModel == nullptr ||
        gAccess.xMin == nullptr || gAccess.xMax == nullptr ||
        gAccess.activeSeriesIndex == nullptr) {
        return;
    }

    gAccess.cursorModel->hideA();
    gAccess.cursorModel->hideB();
    *gAccess.xMin = 0.0;
    *gAccess.xMax = gAccess.dataset->endTimeSeconds();

    for (Series& series : gAccess.dataset->series) {
        series.visible = true;
        series.fitViewToData();
    }

    *gAccess.activeSeriesIndex =
        activeSeries && !gAccess.dataset->series.empty()
            ? 0
            : std::numeric_limits<std::size_t>::max();

    if (gAccess.showCustomLegend != nullptr) {
        *gAccess.showCustomLegend = false;
    }
    if (gAccess.showValues != nullptr) {
        *gAccess.showValues = false;
    }

    ctx->Yield(2);
}

void restoreOverlays(ImGuiTestContext* ctx) {
    if (gAccess.showCustomLegend != nullptr) {
        *gAccess.showCustomLegend = true;
    }
    if (gAccess.showValues != nullptr) {
        *gAccess.showValues = true;
    }
    if (gAccess.cursorModel != nullptr) {
        gAccess.cursorModel->hideA();
        gAccess.cursorModel->hideB();
    }
    ctx->Yield(2);
}

void setCursorAtMouse(ImGuiTestContext* ctx, ImGuiKeyChord modifier) {
    ctx->KeyDown(modifier);
    ctx->Yield();
    ctx->MouseClick(ImGuiMouseButton_Left);
    ctx->Yield(2);
    ctx->KeyUp(modifier);
    ctx->Yield(2);
}
} // namespace

void RegisterCursorTests(ImGuiTestEngine* engine, const GuiTestAccess& access) {
    gAccess = access;

    ImGuiTest* test = IM_REGISTER_TEST(engine, "cursors", "set_a_and_b");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        IM_CHECK(gAccess.dataset != nullptr);
        IM_CHECK(gAccess.cursorModel != nullptr);
        IM_CHECK(gAccess.chartMouseState != nullptr);
        IM_CHECK(gAccess.xMin != nullptr);
        IM_CHECK(gAccess.xMax != nullptr);

        resetState(ctx, false);
        IM_CHECK(moveToPlot(ctx));

        const double xMinBefore = *gAccess.xMin;
        const double xMaxBefore = *gAccess.xMax;
        const double expectedA = gAccess.chartMouseState->timeSeconds;
        setCursorAtMouse(ctx, ImGuiMod_Shift);

        IM_CHECK(gAccess.cursorModel->a().visible);
        IM_CHECK(nearlyEqual(gAccess.cursorModel->a().timeSeconds, expectedA, 1.0e-3));
        IM_CHECK(nearlyEqual(*gAccess.xMin, xMinBefore));
        IM_CHECK(nearlyEqual(*gAccess.xMax, xMaxBefore));

        const ImVec2 currentMouse = ImGui::GetMousePos();
        ctx->MouseTeleportToPos(ImVec2(currentMouse.x + 100.0F, currentMouse.y));
        ctx->Yield(2);
        IM_CHECK(gAccess.chartMouseState->plotHovered);
        const double expectedB = gAccess.chartMouseState->timeSeconds;
        setCursorAtMouse(ctx, ImGuiMod_Ctrl);

        IM_CHECK(gAccess.cursorModel->b().visible);
        IM_CHECK(nearlyEqual(gAccess.cursorModel->b().timeSeconds, expectedB, 1.0e-3));
        IM_CHECK(gAccess.cursorModel->a().visible);
        IM_CHECK(nearlyEqual(*gAccess.xMin, xMinBefore));
        IM_CHECK(nearlyEqual(*gAccess.xMax, xMaxBefore));

        restoreOverlays(ctx);
    };

    test = IM_REGISTER_TEST(engine, "cursors", "plain_drag_and_ctrl_rmb_remove");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        IM_CHECK(gAccess.cursorModel != nullptr);
        IM_CHECK(gAccess.xMin != nullptr);
        IM_CHECK(gAccess.xMax != nullptr);

        resetState(ctx, false);
        IM_CHECK(moveToPlot(ctx));
        setCursorAtMouse(ctx, ImGuiMod_Shift);
        IM_CHECK(gAccess.cursorModel->a().visible);

        const double timeBefore = gAccess.cursorModel->a().timeSeconds;
        const double xMinBefore = *gAccess.xMin;
        const double xMaxBefore = *gAccess.xMax;

        ctx->MouseDragWithDelta(ImVec2(90.0F, 0.0F), ImGuiMouseButton_Left);
        ctx->Yield(2);

        IM_CHECK(!nearlyEqual(gAccess.cursorModel->a().timeSeconds, timeBefore, 1.0e-3));
        IM_CHECK(nearlyEqual(*gAccess.xMin, xMinBefore, 1.0e-4));
        IM_CHECK(nearlyEqual(*gAccess.xMax, xMaxBefore, 1.0e-4));

        ctx->KeyDown(ImGuiMod_Ctrl);
        ctx->Yield();
        ctx->MouseClick(ImGuiMouseButton_Right);
        ctx->Yield(2);
        ctx->KeyUp(ImGuiMod_Ctrl);
        ctx->Yield(2);

        IM_CHECK(!gAccess.cursorModel->a().visible);
        restoreOverlays(ctx);
    };

    test = IM_REGISTER_TEST(engine, "cursors", "alt_drag_over_cursor_pans_y");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        IM_CHECK(gAccess.dataset != nullptr);
        IM_CHECK(gAccess.cursorModel != nullptr);
        IM_CHECK(gAccess.xMin != nullptr);
        IM_CHECK(gAccess.xMax != nullptr);
        IM_CHECK(!gAccess.dataset->series.empty());

        resetState(ctx, true);
        IM_CHECK(moveToPlot(ctx));
        setCursorAtMouse(ctx, ImGuiMod_Shift);
        IM_CHECK(gAccess.cursorModel->a().visible);

        Series& active = gAccess.dataset->series[0];
        const double cursorBefore = gAccess.cursorModel->a().timeSeconds;
        const double xMinBefore = *gAccess.xMin;
        const double xMaxBefore = *gAccess.xMax;
        const float yMinBefore = active.viewMin;
        const float yMaxBefore = active.viewMax;
        const float ySpanBefore = active.viewMax - active.viewMin;

        ctx->KeyDown(ImGuiMod_Alt);
        ctx->Yield();
        ctx->MouseDragWithDelta(ImVec2(75.0F, 45.0F), ImGuiMouseButton_Left);
        ctx->Yield(2);
        ctx->KeyUp(ImGuiMod_Alt);
        ctx->Yield(2);

        IM_CHECK(nearlyEqual(gAccess.cursorModel->a().timeSeconds, cursorBefore, 1.0e-3));
        IM_CHECK(nearlyEqual(*gAccess.xMin, xMinBefore, 1.0e-4));
        IM_CHECK(nearlyEqual(*gAccess.xMax, xMaxBefore, 1.0e-4));
        IM_CHECK(!nearlyEqual(active.viewMin, yMinBefore, 1.0e-5));
        IM_CHECK(!nearlyEqual(active.viewMax, yMaxBefore, 1.0e-5));
        IM_CHECK(nearlyEqual(active.viewMax - active.viewMin, ySpanBefore, 1.0e-4));

        restoreOverlays(ctx);
    };
}
