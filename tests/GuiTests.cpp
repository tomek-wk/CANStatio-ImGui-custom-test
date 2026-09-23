#include "Chart.h"
#include "Dataset.h"
#include "TestEngine.h"

#include <cmath>
#include <limits>

#include <imgui_internal.h>
#include <imgui_test_engine/imgui_te_context.h>

namespace {
GuiTestAccess gAccess;

bool isWindowActive(ImGuiTestContext* ctx, const char* ref) {
    ImGuiWindow* window = ctx->GetWindowByRef(ref);
    return window != nullptr && window->Active;
}

bool isItemDisabled(ImGuiTestContext* ctx, const char* ref) {
    const ImGuiTestItemInfo item = ctx->ItemInfo(ref);
    return (item.ItemFlags & ImGuiItemFlags_Disabled) != 0;
}

bool nearlyEqual(double lhs, double rhs, double epsilon = 1.0e-6) {
    return std::abs(lhs - rhs) <= epsilon;
}

bool samePlotGeometry(const ChartLayoutState& lhs,
                      const ChartLayoutState& rhs,
                      double epsilon = 0.5) {
    return nearlyEqual(lhs.plotX, rhs.plotX, epsilon) &&
           nearlyEqual(lhs.plotY, rhs.plotY, epsilon) &&
           nearlyEqual(lhs.plotWidth, rhs.plotWidth, epsilon) &&
           nearlyEqual(lhs.plotHeight, rhs.plotHeight, epsilon);
}
} // namespace

void RegisterGuiTests(ImGuiTestEngine* engine, const GuiTestAccess& access) {
    gAccess = access;

    ImGuiTest* test = IM_REGISTER_TEST(engine, "custom_chart", "startup_windows");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->Yield();

        IM_CHECK(isWindowActive(ctx, "//ChartHost"));
        IM_CHECK(isWindowActive(ctx, "//Test Controls"));
        IM_CHECK(isWindowActive(ctx, "//Custom Legend"));
        IM_CHECK(isWindowActive(ctx, "//Values"));
    };

    test = IM_REGISTER_TEST(engine, "custom_chart", "window_toggles");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->Yield();
        ctx->SetRef("Test Controls");

        ctx->ItemUncheck("Values");
        ctx->Yield();
        IM_CHECK(!isWindowActive(ctx, "//Values"));

        ctx->ItemCheck("Values");
        ctx->Yield();
        IM_CHECK(isWindowActive(ctx, "//Values"));

        ctx->ItemUncheck("Custom Legend");
        ctx->Yield();
        IM_CHECK(!isWindowActive(ctx, "//Custom Legend"));

        ctx->ItemCheck("Custom Legend");
        ctx->Yield();
        IM_CHECK(isWindowActive(ctx, "//Custom Legend"));
    };

    test = IM_REGISTER_TEST(engine, "custom_chart", "custom_legend_active_series");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        IM_CHECK(gAccess.activeSeriesIndex != nullptr);

        *gAccess.activeSeriesIndex = std::numeric_limits<std::size_t>::max();
        ctx->Yield();

        ctx->SetRef("Test Controls");
        IM_CHECK(isItemDisabled(ctx, "Fit Y"));

        ctx->SetRef("Custom Legend");
        ctx->ItemClick("**/Pedal %");
        ctx->Yield();

        IM_CHECK(*gAccess.activeSeriesIndex == 0);
        ctx->SetRef("Test Controls");
        IM_CHECK(!isItemDisabled(ctx, "Fit Y"));

        ctx->SetRef("Custom Legend");
        ctx->ItemClick("**/Pedal %");
        ctx->Yield();
        IM_CHECK(*gAccess.activeSeriesIndex == std::numeric_limits<std::size_t>::max());
    };

    test = IM_REGISTER_TEST(engine, "custom_chart", "active_highlight_mode");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        IM_CHECK(gAccess.activeHighlightMode != nullptr);

        *gAccess.activeHighlightMode = ActiveHighlightMode::Halo;
        ctx->Yield();

        ctx->SetRef("Test Controls");
        ctx->ItemClick("Outline");
        ctx->Yield();
        IM_CHECK(*gAccess.activeHighlightMode == ActiveHighlightMode::Outline);

        ctx->ItemClick("Halo");
        ctx->Yield();
        IM_CHECK(*gAccess.activeHighlightMode == ActiveHighlightMode::Halo);
    };

    test = IM_REGISTER_TEST(engine, "custom_chart", "plot_geometry_and_mouse_time");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        IM_CHECK(gAccess.dataset != nullptr);
        IM_CHECK(gAccess.activeSeriesIndex != nullptr);
        IM_CHECK(gAccess.xMin != nullptr);
        IM_CHECK(gAccess.xMax != nullptr);
        IM_CHECK(gAccess.chartMouseState != nullptr);
        IM_CHECK(gAccess.chartLayoutState != nullptr);

        *gAccess.activeSeriesIndex = std::numeric_limits<std::size_t>::max();
        *gAccess.xMin = 0.0;
        *gAccess.xMax = gAccess.dataset->endTimeSeconds();
        ctx->Yield(2);

        const ChartLayoutState noActive = *gAccess.chartLayoutState;
        IM_CHECK(noActive.plotWidth > 100.0F);
        IM_CHECK(noActive.plotHeight > 100.0F);

        ImGuiWindow* chartHost = ImGui::FindWindowByName("ChartHost");
        IM_CHECK(chartHost != nullptr);
        ctx->MouseSetViewport(chartHost);

        const ImVec2 center(noActive.plotX + 0.5F * noActive.plotWidth,
                            noActive.plotY + 0.5F * noActive.plotHeight);
        ctx->MouseTeleportToPos(center);
        ctx->Yield(2);

        IM_CHECK(gAccess.chartMouseState->plotHovered);
        IM_CHECK(gAccess.chartMouseState->withinDataset);
        const double expectedTime = 0.5 * (*gAccess.xMin + *gAccess.xMax);
        IM_CHECK(nearlyEqual(gAccess.chartMouseState->timeSeconds, expectedTime, 0.25));

        *gAccess.activeSeriesIndex = 0;
        ctx->Yield(2);
        IM_CHECK(samePlotGeometry(noActive, *gAccess.chartLayoutState));

        *gAccess.activeSeriesIndex = std::numeric_limits<std::size_t>::max();
        ctx->Yield();
    };

    test = IM_REGISTER_TEST(engine, "custom_chart", "fit_x");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        IM_CHECK(gAccess.dataset != nullptr);
        IM_CHECK(gAccess.xMin != nullptr);
        IM_CHECK(gAccess.xMax != nullptr);

        const double dataEnd = gAccess.dataset->endTimeSeconds();
        *gAccess.xMin = dataEnd * 0.20;
        *gAccess.xMax = dataEnd * 0.55;
        ctx->Yield();

        ctx->SetRef("Test Controls");
        ctx->ItemClick("Fit X");
        ctx->Yield();

        IM_CHECK(nearlyEqual(*gAccess.xMin, 0.0));
        IM_CHECK(nearlyEqual(*gAccess.xMax, dataEnd));
    };
}
