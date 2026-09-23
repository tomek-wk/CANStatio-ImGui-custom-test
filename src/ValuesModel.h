#pragma once

#include <cstddef>
#include <vector>

struct ChartMouseState;
struct Dataset;

struct ValuesRow {
    std::size_t seriesIndex = 0;
    double value = 0.0;
};

struct ValuesSnapshot {
    bool plotHovered = false;
    bool withinDataset = false;
    double timeSeconds = 0.0;
    std::vector<ValuesRow> rows;
};

[[nodiscard]] ValuesSnapshot makeValuesSnapshot(const Dataset& dataset,
                                                const ChartMouseState& mouseState);
