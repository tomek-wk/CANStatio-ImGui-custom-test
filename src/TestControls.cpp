#include "TestControls.h"

#include "Chart.h"
#include "Dataset.h"

#include <imgui.h>

void TestControls::draw(Dataset& dataset,
                        std::size_t& activeSeriesIndex,
                        ActiveHighlightMode& activeHighlightMode,
                        double& xMin,
                        double& xMax,
                        bool& showNativeLegend,
                        bool& showCustomLegend,
                        bool& showValues,
                        bool& showCrosshair) {
    ImGui::SetNextWindowSize(ImVec2(360.0F, 500.0F), ImGuiCond_FirstUseEver);
    ImGui::Begin("Test Controls");

    const ImGuiIO& io = ImGui::GetIO();
    const float fps = io.Framerate;
    const float frameMs = fps > 0.0F ? 1000.0F / fps : 0.0F;

    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("Frame: %.2f ms", frameMs);
    ImGui::Separator();

    ImGui::TextUnformatted("Preset: Small / Sanity");
    ImGui::Text("Series: %zu", dataset.series.size());
    ImGui::Text("Samples: %zu", dataset.sampleCount);
    ImGui::Text("dt: %.0f ms", dataset.dtSeconds * 1000.0);
    ImGui::Text("Data end: %.1f s", dataset.endTimeSeconds());
    ImGui::Text("X view: %.3f .. %.3f s", xMin, xMax);

    if (ImGui::Button("Fit X")) {
        xMin = 0.0;
        xMax = dataset.endTimeSeconds();
    }

    ImGui::Separator();

    const Series* activeSeries = nullptr;
    if (activeSeriesIndex < dataset.series.size()) {
        activeSeries = &dataset.series[activeSeriesIndex];
    }

    ImGui::Text("Active: %s", activeSeries != nullptr ? activeSeries->name.c_str() : "<none>");

    const bool hasActiveVisible = activeSeries != nullptr && activeSeries->visible;
    ImGui::BeginDisabled(!hasActiveVisible);
    if (ImGui::Button("Fit Y") && hasActiveVisible) {
        dataset.series[activeSeriesIndex].fitViewToData();
    }
    ImGui::EndDisabled();

    if (activeSeries != nullptr) {
        ImGui::Text("Y view: %.3f .. %.3f", activeSeries->viewMin, activeSeries->viewMax);
        if (!activeSeries->visible) {
            ImGui::TextUnformatted("Active series is hidden");
        }
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Active highlight");
    if (ImGui::RadioButton("Halo", activeHighlightMode == ActiveHighlightMode::Halo)) {
        activeHighlightMode = ActiveHighlightMode::Halo;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Outline", activeHighlightMode == ActiveHighlightMode::Outline)) {
        activeHighlightMode = ActiveHighlightMode::Outline;
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Windows / overlays");
    ImGui::Checkbox("Values", &showValues);
    ImGui::Checkbox("Crosshair", &showCrosshair);
    ImGui::Checkbox("Native ImPlot Legend", &showNativeLegend);
    ImGui::Checkbox("Custom Legend", &showCustomLegend);

    ImGui::Separator();
    ImGui::TextDisabled("Shift + LMB: set cursor A");
    ImGui::TextDisabled("Ctrl + LMB: set cursor B");
    ImGui::TextDisabled("LMB drag cursor: move A/B");
    ImGui::TextDisabled("Ctrl + RMB cursor: hide A/B");
    ImGui::TextDisabled("RMB near a line: toggle active");
    ImGui::TextDisabled("RMB outside lines: clear active");
    ImGui::TextDisabled("Alt + drag: pan active Y");
    ImGui::TextDisabled("Alt + wheel: zoom active Y");
    ImGui::TextDisabled("Plain drag / wheel: native X");

    ImGui::End();
}
