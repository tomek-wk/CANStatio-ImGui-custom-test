#pragma once

#include <cstddef>

struct Dataset;

class CustomLegend {
public:
    static void draw(Dataset& dataset,
                     std::size_t& activeSeriesIndex,
                     bool& visible);
};
