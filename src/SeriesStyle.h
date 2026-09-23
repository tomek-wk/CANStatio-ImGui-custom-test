#pragma once

#include <cmath>
#include <cstddef>

#include <imgui.h>

namespace SeriesStyle {

inline ImVec4 color(std::size_t index) {
    constexpr float goldenRatioConjugate = 0.61803398875F;
    const float hue = std::fmod(static_cast<float>(index) * goldenRatioConjugate, 1.0F);

    float r = 0.0F;
    float g = 0.0F;
    float b = 0.0F;
    ImGui::ColorConvertHSVtoRGB(hue, 0.65F, 0.95F, r, g, b);
    return ImVec4(r, g, b, 1.0F);
}

} // namespace SeriesStyle
