#include "CursorOverlay.h"

#include "CursorModel.h"
#include "Dataset.h"

#include <cmath>

#include <imgui.h>
#include <implot.h>

namespace {
constexpr float kCursorThicknessPixels = 2.0F;
constexpr float kCursorHitTolerancePixels = 6.0F;
constexpr ImVec4 kCursorAColor(1.0F, 0.72F, 0.12F, 1.0F);
constexpr ImVec4 kCursorBColor(0.10F, 0.80F, 1.0F, 1.0F);

bool cursorNearMouse(const TimeCursor& cursor) {
    if (!cursor.visible || !ImPlot::IsPlotHovered()) {
        return false;
    }

    const ImVec2 cursorPixel = ImPlot::PlotToPixels(cursor.timeSeconds, 0.0, ImAxis_X1, ImAxis_Y1);
    return std::abs(ImGui::GetMousePos().x - cursorPixel.x) <= kCursorHitTolerancePixels;
}

void handleSetInput(const Dataset& dataset, CursorModel& cursors) {
    if (!ImPlot::IsPlotHovered() || !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        return;
    }

    const ImGuiKeyChord modifiers = ImGui::GetIO().KeyMods;
    const double mouseTime = ImPlot::GetPlotMousePos(ImAxis_X1, ImAxis_Y1).x;
    const double datasetEnd = dataset.endTimeSeconds();

    if (modifiers == ImGuiMod_Shift) {
        cursors.setA(mouseTime, datasetEnd);
    } else if (modifiers == ImGuiMod_Ctrl) {
        cursors.setB(mouseTime, datasetEnd);
    }
}

void handleRemoveInput(CursorModel& cursors) {
    if (ImGui::GetIO().KeyMods != ImGuiMod_Ctrl ||
        !ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
        !ImPlot::IsPlotHovered()) {
        return;
    }

    if (cursorNearMouse(cursors.a())) {
        cursors.hideA();
    } else if (cursorNearMouse(cursors.b())) {
        cursors.hideB();
    }
}

void drawLabel(const TimeCursor& cursor, const ImVec4& color, const char* label) {
    if (!cursor.visible) {
        return;
    }

    const ImVec2 plotPos = ImPlot::GetPlotPos();
    const ImVec2 pixel = ImPlot::PlotToPixels(cursor.timeSeconds, 0.0, ImAxis_X1, ImAxis_Y1);

    ImPlot::PushPlotClipRect();
    ImPlot::GetPlotDrawList()->AddText(
        ImVec2(pixel.x + 4.0F, plotPos.y + 4.0F),
        ImGui::ColorConvertFloat4ToU32(color),
        label);
    ImPlot::PopPlotClipRect();
}

bool drawCursor(TimeCursor& cursor,
                int id,
                const ImVec4& color,
                double datasetEndSeconds,
                bool allowDrag) {
    if (!cursor.visible) {
        return false;
    }

    bool held = false;
    ImPlotDragToolFlags flags = ImPlotDragToolFlags_NoFit;
    if (!allowDrag) {
        flags |= ImPlotDragToolFlags_NoInputs;
    }

    if (ImPlot::DragLineX(id,
                          &cursor.timeSeconds,
                          color,
                          kCursorThicknessPixels,
                          flags,
                          nullptr,
                          nullptr,
                          &held)) {
        cursor.timeSeconds = CursorModel::clampTime(cursor.timeSeconds, datasetEndSeconds);
    }

    return held;
}
} // namespace

bool CursorOverlay::draw(const Dataset& dataset, CursorModel& cursors) {
    if (dataset.sampleCount == 0) {
        return false;
    }

    handleSetInput(dataset, cursors);
    handleRemoveInput(cursors);

    const bool allowDrag = ImGui::GetIO().KeyMods == ImGuiMod_None;
    const double datasetEnd = dataset.endTimeSeconds();

    const bool aHeld = drawCursor(cursors.a(), 0xCA01, kCursorAColor, datasetEnd, allowDrag);
    const bool bHeld = drawCursor(cursors.b(), 0xCA02, kCursorBColor, datasetEnd, allowDrag);

    drawLabel(cursors.a(), kCursorAColor, "A");
    drawLabel(cursors.b(), kCursorBColor, "B");

    return aHeld || bHeld;
}
