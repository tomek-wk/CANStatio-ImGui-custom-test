#include "CustomLegend.h"

#include "Dataset.h"
#include "SeriesStyle.h"

#include <limits>

#include <imgui.h>

void CustomLegend::draw(Dataset& dataset,
                        std::size_t& activeSeriesIndex,
                        bool& visible) {
    if (!visible) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(300.0F, 280.0F), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Custom Legend", &visible)) {
        ImGui::End();
        return;
    }

    for (std::size_t i = 0; i < dataset.series.size(); ++i) {
        Series& series = dataset.series[i];
        ImGui::PushID(static_cast<int>(i));

        ImGui::Checkbox("##visible", &series.visible);
        ImGui::SameLine();

        const ImVec4 color = SeriesStyle::color(i);
        ImGui::ColorButton("##color",
                           color,
                           ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop,
                           ImVec2(14.0F, 14.0F));
        ImGui::SameLine();

        const bool selected = activeSeriesIndex == i;
        if (ImGui::Selectable(series.name.c_str(), selected)) {
            activeSeriesIndex = selected
                ? std::numeric_limits<std::size_t>::max()
                : i;
        }

        ImGui::PopID();
    }

    ImGui::End();
}
