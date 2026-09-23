#include "DataGenerator.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <utility>

namespace {
constexpr std::size_t kSmallSampleCount = 1000;
constexpr double kSmallDtSeconds = 0.1;
constexpr unsigned int kSmallSeed = 0xCA57A710U;

void finishSeries(Series& series) {
    const auto [minIt, maxIt] = std::minmax_element(series.values.begin(), series.values.end());
    if (minIt == series.values.end()) {
        series.dataMin = 0.0F;
        series.dataMax = 0.0F;
        series.fitViewToData();
        return;
    }

    series.dataMin = *minIt;
    series.dataMax = *maxIt;
    series.fitViewToData();
}
} // namespace

Dataset DataGenerator::makeSmall() {
    Dataset dataset;
    dataset.dtSeconds = kSmallDtSeconds;
    dataset.sampleCount = kSmallSampleCount;
    dataset.series = {
        Series{.name = "Pedal %", .values = {}},
        Series{.name = "Throttle %", .values = {}},
        Series{.name = "Load %", .values = {}},
    };

    for (Series& series : dataset.series) {
        series.values.resize(dataset.sampleCount);
    }

    std::mt19937 rng(kSmallSeed);
    std::normal_distribution<float> noise(0.0F, 0.45F);

    for (std::size_t i = 0; i < dataset.sampleCount; ++i) {
        const double t = static_cast<double>(i) * dataset.dtSeconds;

        const float pedal = 50.0F + 30.0F * static_cast<float>(std::sin(0.11 * t)) + noise(rng);
        const float throttle = 47.0F
            + 25.0F * static_cast<float>(std::sin(0.11 * t + 0.65))
            + 5.0F * static_cast<float>(std::sin(0.48 * t));
        const float load = 42.0F
            + 21.0F * static_cast<float>(std::sin(0.075 * t + 1.35))
            + 4.0F * static_cast<float>(std::sin(0.31 * t + 0.2))
            + noise(rng);

        dataset.series[0].values[i] = std::clamp(pedal, 0.0F, 100.0F);
        dataset.series[1].values[i] = std::clamp(throttle, 0.0F, 100.0F);
        dataset.series[2].values[i] = std::clamp(load, 0.0F, 100.0F);
    }

    for (Series& series : dataset.series) {
        finishSeries(series);
    }

    return dataset;
}
