#include "ValuesWindow.h"

#include "Chart.h"
#include "Dataset.h"
#include "SeriesStyle.h"
#include "ValuesModel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include <imgui.h>

namespace {
std::string formatTime(double seconds) {
    const long long totalMilliseconds = std::llround(std::max(seconds, 0.0) * 1000.0);
    const long long milliseconds = totalMilliseconds % 1000;
    const long long totalSeconds = totalMilliseconds / 1000;
    const long long secondsPart = totalSeconds % 60;
    const long long totalMinutes = totalSeconds / 60;
    const long long minutesPart = totalMinutes % 60;
    const long long hours = totalMinutes / 60;

    char buffer[64]{};
    if (hours > 0) {
        std::snprintf(buffer,
                      sizeof(buffer),
                      "%02lld:%02lld:%02lld.%03lld",
                      hours,
                      minutesPart,
                      secondsPart,
                      milliseconds);
    } else {
        std::snprintf(buffer,
                      sizeof(buffer),
                      "%02lld:%02lld.%03lld",
                      totalMinutes,
                      secondsPart,
                      milliseconds);
    }
    return buffer;
}

void drawText(const std::string& text, bool bold) {
    if (!bold) {
        ImGui::TextUnformatted(text.c_str());
        return;
    }

    const ImVec2 position = ImGui::GetCursorScreenPos();
    const ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddText(position, color, text.c_str());
    drawList->AddText(ImVec2(position.x + 0.8F, position.y), color, text.c_str());
    ImGui::Dummy(ImGui::CalcTextSize(text.c_str()));
}
} // namespace

void ValuesWindow::draw(const Dataset& dataset,
                        std::size_t activeSeriesIndex,
                        const ChartMouseState& mouseState,
                        bool& visible) {
    if (!visible) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(300.0F, 260.0F), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Values", &visible)) {
        ImGui::End();
        return;
    }

    const ValuesSnapshot snapshot = makeValuesSnapshot(dataset, mouseState);

    if (!snapshot.plotHovered) {
        ImGui::TextDisabled("Move mouse over chart");
        ImGui::End();
        return;
    }

    if (!snapshot.withinDataset) {
        ImGui::Text("Time: %.3f s", snapshot.timeSeconds);
        ImGui::TextDisabled("Outside dataset");
        ImGui::End();
        return;
    }

    const std::string timeText = formatTime(snapshot.timeSeconds);
    ImGui::Text("Time: %s", timeText.c_str());
    ImGui::Separator();

    for (const ValuesRow& row : snapshot.rows) {
        const std::size_t i = row.seriesIndex;
        const Series& series = dataset.series[i];

        ImGui::PushID(static_cast<int>(i));
        ImGui::ColorButton("##color",
                           SeriesStyle::color(i),
                           ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop,
                           ImVec2(14.0F, 14.0F));
        ImGui::SameLine();

        const int decimals =
            (static_cast<double>(series.dataMax) - series.dataMin) < 1.0 ? 3 : 2;
        char buffer[256]{};
        std::snprintf(buffer,
                      sizeof(buffer),
                      decimals == 3 ? "%s: %.3f" : "%s: %.2f",
                      series.name.c_str(),
                      row.value);
        drawText(buffer, activeSeriesIndex == i);
        ImGui::PopID();
    }

    ImGui::End();
}
