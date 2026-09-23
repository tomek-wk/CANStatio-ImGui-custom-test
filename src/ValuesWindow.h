#pragma once

#include <cstddef>

struct ChartMouseState;
struct Dataset;

class ValuesWindow {
public:
    static void draw(const Dataset& dataset,
                     std::size_t activeSeriesIndex,
                     const ChartMouseState& mouseState,
                     bool& visible);
};
