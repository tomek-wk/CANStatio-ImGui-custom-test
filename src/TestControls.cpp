#include "TestControls.h"

#include "Chart.h"
#include "CursorModel.h"
#include "DataGenerator.h"
#include "Dataset.h"
#include "MarkerModel.h"

#include <algorithm>
#include <array>
#include <limits>

#include <imgui.h>

namespace {
constexpr std::size_t kNoIndex = std::numeric_limits<std::size_t>::max();

void resetExperiment(Dataset& dataset,
                     std::size_t& activeSeriesIndex,
                     double& xMin,
                     double& xMax,
                     CursorModel& cursorModel,
                     MarkerModel& markerModel) {
    xMin = 0.0;
    xMax = dataset.endTimeSeconds();
    if (!(xMax > xMin)) {
        xMax = xMin + std::max(dataset.dtSeconds, 1.0e-6);
    }
    activeSeriesIndex = kNoIndex;
    for (Series& series : dataset.series) {
        series.visible = true;
        series.fitViewToData();
    }
    cursorModel.reset();
    markerModel.reset();
}
} // namespace

bool TestControls::draw(Dataset& dataset,
                        DataPreset& preset,
                        std::size_t& activeSeriesIndex,
                        ActiveHighlightMode& activeHighlightMode,
                        double& xMin,
                        double& xMax,
                        CursorModel& cursorModel,
                        MarkerModel& markerModel,
                        bool& showCustomLegend,
                        bool& showValues,
                        bool& showCrosshair) {
    bool fullReset = false;

    ImGui::SetNextWindowSize(ImVec2(380.0F, 620.0F), ImGuiCond_FirstUseEver);
    ImGui::Begin("Test Controls");

    const ImGuiIO& io = ImGui::GetIO();
    const float fps = io.Framerate;
    const float frameMs = fps > 0.0F ? 1000.0F / fps : 0.0F;
    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("Frame: %.2f ms", frameMs);
    ImGui::Separator();
    ImGui::TextUnformatted("Renderer: custom Dear ImGui draw list");
    ImGui::TextUnformatted("Stage: full custom-chart experiment");
    ImGui::Separator();

    constexpr int presetCount = static_cast<int>(DataPreset::Count);
    std::array<const char*, presetCount> presetNames{};
    for (int i = 0; i < presetCount; ++i) {
        presetNames[static_cast<std::size_t>(i)] = dataPresetName(static_cast<DataPreset>(i));
    }

    int presetIndex = static_cast<int>(preset);
    if (ImGui::Combo("Preset", &presetIndex, presetNames.data(), presetCount)) {
        preset = static_cast<DataPreset>(presetIndex);
        dataset = DataGenerator::make(preset);
        resetExperiment(dataset, activeSeriesIndex, xMin, xMax, cursorModel, markerModel);
        fullReset = true;
    }

    ImGui::Text("Series: %zu", dataset.series.size());
    ImGui::Text("Samples: %zu", dataset.sampleCount);
    ImGui::Text("dt: %.3f ms", dataset.dtSeconds * 1000.0);
    ImGui::Text("Data end: %.3f s", dataset.endTimeSeconds());
    ImGui::Text("X view: %.6f .. %.6f s", xMin, xMax);
    ImGui::Text("Markers: %zu", markerModel.markers().size());

    if (ImGui::Button("Fit X")) {
        xMin = 0.0;
        xMax = dataset.endTimeSeconds();
        if (!(xMax > xMin)) {
            xMax = xMin + std::max(dataset.dtSeconds, 1.0e-6);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        resetExperiment(dataset, activeSeriesIndex, xMin, xMax, cursorModel, markerModel);
        fullReset = true;
    }

    ImGui::Separator();

    const Series* activeSeries = activeSeriesIndex < dataset.series.size()
        ? &dataset.series[activeSeriesIndex]
        : nullptr;
    ImGui::Text("Active: %s", activeSeries != nullptr ? activeSeries->name.c_str() : "<none>");

    const bool hasActiveVisible = activeSeries != nullptr && activeSeries->visible;
    ImGui::BeginDisabled(!hasActiveVisible);
    if (ImGui::Button("Fit Y") && hasActiveVisible) {
        dataset.series[activeSeriesIndex].fitViewToData();
    }
    ImGui::EndDisabled();

    if (activeSeries != nullptr) {
        ImGui::Text("Y view: %.6f .. %.6f", activeSeries->viewMin, activeSeries->viewMax);
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
    ImGui::Checkbox("Custom Legend", &showCustomLegend);

    ImGui::Separator();
    ImGui::TextDisabled("Plain LMB drag: pan X");
    ImGui::TextDisabled("Wheel: zoom X at mouse time");
    ImGui::TextDisabled("RMB near line: toggle active");
    ImGui::TextDisabled("Alt + drag: pan active Y");
    ImGui::TextDisabled("Alt + wheel: zoom active Y");
    ImGui::TextDisabled("Shift + LMB: set cursor A");
    ImGui::TextDisabled("Ctrl + LMB: set cursor B");
    ImGui::TextDisabled("Plain LMB on cursor/marker: drag");
    ImGui::TextDisabled("Ctrl + RMB on cursor/marker: hide/remove");
    ImGui::TextDisabled("Alt + click: add marker");

    ImGui::End();
    return fullReset;
}
