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

    ImGuiTest* test = IM_REGISTER_TEST(engine, "canstatio", "startup_windows");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->Yield();

        IM_CHECK(isWindowActive(ctx, "//Test Controls"));
        IM_CHECK(isWindowActive(ctx, "//Custom Legend"));
        IM_CHECK(isWindowActive(ctx, "//Values"));
    };

    test = IM_REGISTER_TEST(engine, "canstatio", "window_toggles");
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

    test = IM_REGISTER_TEST(engine, "canstatio", "custom_legend_active_series");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->Yield();

        ctx->SetRef("Test Controls");
        IM_CHECK(isItemDisabled(ctx, "Fit Y"));

        ctx->SetRef("Custom Legend");
        ctx->ItemClick("**/Pedal %");
        ctx->Yield();
        ctx->SetRef("Test Controls");
        IM_CHECK(!isItemDisabled(ctx, "Fit Y"));

        ctx->SetRef("Custom Legend");
        ctx->ItemClick("**/Throttle %");
        ctx->Yield();
        ctx->SetRef("Test Controls");
        IM_CHECK(!isItemDisabled(ctx, "Fit Y"));

        // Switching back to Pedal must keep an active visible series. If the
        // previous click failed to change active, this click would clear it.
        ctx->SetRef("Custom Legend");
        ctx->ItemClick("**/Pedal %");
        ctx->Yield();
        ctx->SetRef("Test Controls");
        IM_CHECK(!isItemDisabled(ctx, "Fit Y"));

        // Clicking the currently active series again clears active.
        ctx->SetRef("Custom Legend");
        ctx->ItemClick("**/Pedal %");
        ctx->Yield();
        ctx->SetRef("Test Controls");
        IM_CHECK(isItemDisabled(ctx, "Fit Y"));
    };

    test = IM_REGISTER_TEST(engine, "canstatio", "custom_legend_hidden_active");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->Yield();

        ctx->SetRef("Custom Legend");
        IM_CHECK(ctx->ItemIsChecked("$$0/##visible"));
        ctx->ItemClick("**/Pedal %");
        ctx->Yield();

        ctx->SetRef("Test Controls");
        IM_CHECK(!isItemDisabled(ctx, "Fit Y"));

        // Hiding the active series must keep it logically active. Fit Y is
        // disabled only because the active series is currently hidden.
        ctx->SetRef("Custom Legend");
        ctx->ItemUncheck("$$0/##visible");
        ctx->Yield();
        IM_CHECK(!ctx->ItemIsChecked("$$0/##visible"));

        ctx->SetRef("Test Controls");
        IM_CHECK(isItemDisabled(ctx, "Fit Y"));

        // Showing the same series again must re-enable Fit Y without another
        // click on its name, proving that active survived hide/show.
        ctx->SetRef("Custom Legend");
        ctx->ItemCheck("$$0/##visible");
        ctx->Yield();
        IM_CHECK(ctx->ItemIsChecked("$$0/##visible"));

        ctx->SetRef("Test Controls");
        IM_CHECK(!isItemDisabled(ctx, "Fit Y"));

        // Restore the neutral no-active state for later tests.
        ctx->SetRef("Custom Legend");
        ctx->ItemClick("**/Pedal %");
        ctx->Yield();
        ctx->SetRef("Test Controls");
        IM_CHECK(isItemDisabled(ctx, "Fit Y"));
    };

    test = IM_REGISTER_TEST(engine, "canstatio", "active_highlight_mode");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->Yield();
        IM_CHECK(gAccess.activeHighlightMode != nullptr);
        IM_CHECK(*gAccess.activeHighlightMode == ActiveHighlightMode::Halo);

        ctx->SetRef("Test Controls");
        ctx->ItemClick("Outline");
        ctx->Yield();
        IM_CHECK(*gAccess.activeHighlightMode == ActiveHighlightMode::Outline);

        // Restore the default mode for later tests and manual runs.
        ctx->ItemClick("Halo");
        ctx->Yield();
        IM_CHECK(*gAccess.activeHighlightMode == ActiveHighlightMode::Halo);
    };

    test = IM_REGISTER_TEST(engine, "canstatio", "stable_plot_area_active_y_axis");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        IM_CHECK(gAccess.dataset != nullptr);
        IM_CHECK(gAccess.activeSeriesIndex != nullptr);
        IM_CHECK(gAccess.chartLayoutState != nullptr);
        IM_CHECK(!gAccess.dataset->series.empty());

        for (Series& series : gAccess.dataset->series) {
            series.visible = true;
        }

        *gAccess.activeSeriesIndex = std::numeric_limits<std::size_t>::max();
        ctx->Yield(2);
        const ChartLayoutState noActive = *gAccess.chartLayoutState;
        IM_CHECK(noActive.plotWidth > 0.0F);
        IM_CHECK(noActive.plotHeight > 0.0F);

        *gAccess.activeSeriesIndex = 0;
        ctx->Yield(2);
        const ChartLayoutState firstActive = *gAccess.chartLayoutState;
        IM_CHECK(samePlotGeometry(noActive, firstActive));

        if (gAccess.dataset->series.size() > 1) {
            *gAccess.activeSeriesIndex = gAccess.dataset->series.size() - 1;
            ctx->Yield(2);
            const ChartLayoutState differentScaleActive = *gAccess.chartLayoutState;
            IM_CHECK(samePlotGeometry(noActive, differentScaleActive));
        }

        *gAccess.activeSeriesIndex = std::numeric_limits<std::size_t>::max();
        ctx->Yield(2);
        IM_CHECK(samePlotGeometry(noActive, *gAccess.chartLayoutState));
    };

    test = IM_REGISTER_TEST(engine, "canstatio", "fit_x");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->Yield();
        IM_CHECK(gAccess.dataset != nullptr);
        IM_CHECK(gAccess.xMin != nullptr);
        IM_CHECK(gAccess.xMax != nullptr);

        const double dataEnd = gAccess.dataset->endTimeSeconds();
        IM_CHECK(dataEnd > 0.0);

        *gAccess.xMin = dataEnd * 0.20;
        *gAccess.xMax = dataEnd * 0.55;
        ctx->Yield();
        IM_CHECK(!nearlyEqual(*gAccess.xMin, 0.0));
        IM_CHECK(!nearlyEqual(*gAccess.xMax, dataEnd));

        ctx->SetRef("Test Controls");
        ctx->ItemClick("Fit X");
        ctx->Yield();

        IM_CHECK(nearlyEqual(*gAccess.xMin, 0.0));
        IM_CHECK(nearlyEqual(*gAccess.xMax, dataEnd));
    };

    test = IM_REGISTER_TEST(engine, "canstatio", "fit_y");
    test->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->Yield();
        IM_CHECK(gAccess.dataset != nullptr);
        IM_CHECK(!gAccess.dataset->series.empty());

        ctx->SetRef("Custom Legend");
        ctx->ItemClick("**/Pedal %");
        ctx->Yield();

        Series& series = gAccess.dataset->series[0];
        const float span = series.dataMax - series.dataMin;
        float expectedMin = 0.0F;
        float expectedMax = 0.0F;
        if (span > 0.0F) {
            const float margin = span * 0.05F;
            expectedMin = series.dataMin - margin;
            expectedMax = series.dataMax + margin;
            series.viewMin = series.dataMin + span * 0.20F;
            series.viewMax = series.dataMax - span * 0.20F;
        } else {
            const float scale = std::max(std::abs(series.dataMin), 1.0F);
            const float halfRange = scale * 0.05F;
            expectedMin = series.dataMin - halfRange;
            expectedMax = series.dataMax + halfRange;
            series.viewMin = series.dataMin - halfRange * 0.50F;
            series.viewMax = series.dataMax + halfRange * 0.50F;
        }

        ctx->SetRef("Test Controls");
        ctx->ItemClick("Fit Y");
        ctx->Yield();

        IM_CHECK(nearlyEqual(series.viewMin, expectedMin, 1.0e-4));
        IM_CHECK(nearlyEqual(series.viewMax, expectedMax, 1.0e-4));

        // Restore neutral active state while preserving the fitted Y range.
        ctx->SetRef("Custom Legend");
        ctx->ItemClick("**/Pedal %");
        ctx->Yield();
    };
}
