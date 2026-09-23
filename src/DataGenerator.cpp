#include "DataGenerator.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <string>

namespace {
constexpr unsigned int kSeed = 0xCA57A710U;

void finishSeries(Series& series) {
    const auto [minIt, maxIt] = std::minmax_element(series.values.begin(), series.values.end());
    if (minIt == series.values.end()) {
        series.dataMin = 0.0F;
        series.dataMax = 0.0F;
    } else {
        series.dataMin = *minIt;
        series.dataMax = *maxIt;
    }
    series.fitViewToData();
}

void finishDataset(Dataset& dataset) {
    for (Series& series : dataset.series) {
        finishSeries(series);
    }
}

Dataset makeBase(std::size_t seriesCount,
                 std::size_t sampleCount,
                 double dtSeconds,
                 const std::string& prefix = "Series ") {
    Dataset dataset;
    dataset.dtSeconds = dtSeconds;
    dataset.sampleCount = sampleCount;
    dataset.series.resize(seriesCount);
    for (std::size_t s = 0; s < seriesCount; ++s) {
        dataset.series[s].name = prefix + std::to_string(s + 1);
        dataset.series[s].values.resize(sampleCount);
    }
    return dataset;
}

Dataset makeSmallPreset() {
    Dataset dataset = makeBase(3, 1000, 0.1);
    dataset.series[0].name = "Pedal %";
    dataset.series[1].name = "Throttle %";
    dataset.series[2].name = "Load %";

    std::mt19937 rng(kSeed);
    std::normal_distribution<float> noise(0.0F, 0.45F);

    for (std::size_t i = 0; i < dataset.sampleCount; ++i) {
        const double t = static_cast<double>(i) * dataset.dtSeconds;
        dataset.series[0].values[i] = std::clamp(
            50.0F + 30.0F * static_cast<float>(std::sin(0.11 * t)) + noise(rng),
            0.0F,
            100.0F);
        dataset.series[1].values[i] = std::clamp(
            47.0F + 25.0F * static_cast<float>(std::sin(0.11 * t + 0.65))
                + 5.0F * static_cast<float>(std::sin(0.48 * t)),
            0.0F,
            100.0F);
        dataset.series[2].values[i] = std::clamp(
            42.0F + 21.0F * static_cast<float>(std::sin(0.075 * t + 1.35))
                + 4.0F * static_cast<float>(std::sin(0.31 * t + 0.2))
                + noise(rng),
            0.0F,
            100.0F);
    }

    finishDataset(dataset);
    return dataset;
}

Dataset makeReferencePreset(std::size_t seriesCount,
                            std::size_t sampleCount,
                            double dtSeconds) {
    Dataset dataset = makeBase(seriesCount, sampleCount, dtSeconds);
    for (std::size_t s = 0; s < seriesCount; ++s) {
        const double base = 15.0 + static_cast<double>(s % 10) * 7.5;
        const double amplitude = 4.0 + static_cast<double>(s % 8) * 2.6;
        const double frequency = 0.006 + static_cast<double>(s % 13) * 0.0011;
        const double phase = static_cast<double>(s) * 0.37;
        for (std::size_t i = 0; i < sampleCount; ++i) {
            const double t = static_cast<double>(i) * dtSeconds;
            dataset.series[s].values[i] = static_cast<float>(
                base
                + amplitude * std::sin(frequency * t + phase)
                + amplitude * 0.28 * std::sin(frequency * 4.7 * t + phase * 0.4));
        }
    }
    finishDataset(dataset);
    return dataset;
}

Dataset makeOverlapPreset() {
    Dataset dataset = makeBase(60, 36000, 0.1);
    for (std::size_t s = 0; s < dataset.series.size(); ++s) {
        const double phase = static_cast<double>(s) * 0.21;
        const double offset = static_cast<double>(static_cast<int>(s % 7) - 3) * 0.8;
        for (std::size_t i = 0; i < dataset.sampleCount; ++i) {
            const double t = static_cast<double>(i) * dataset.dtSeconds;
            dataset.series[s].values[i] = static_cast<float>(
                50.0 + offset
                + 26.0 * std::sin(0.016 * t + phase)
                + 8.0 * std::sin(0.071 * t + phase * 1.9));
        }
    }
    finishDataset(dataset);
    return dataset;
}

Dataset makeMixedScalePreset() {
    Dataset dataset = makeBase(60, 36000, 0.1);
    for (std::size_t s = 0; s < dataset.series.size(); ++s) {
        const int group = static_cast<int>(s % 4);
        const double phase = static_cast<double>(s) * 0.31;
        if (group == 0) {
            dataset.series[s].name = "RPM-like " + std::to_string(s + 1);
        } else if (group == 1) {
            dataset.series[s].name = "Percent-like " + std::to_string(s + 1);
        } else if (group == 2) {
            dataset.series[s].name = "Signed-small " + std::to_string(s + 1);
        } else {
            dataset.series[s].name = "Precision " + std::to_string(s + 1);
        }

        for (std::size_t i = 0; i < dataset.sampleCount; ++i) {
            const double t = static_cast<double>(i) * dataset.dtSeconds;
            const double wave = std::sin(0.012 * t + phase) + 0.2 * std::sin(0.083 * t + phase * 0.6);
            double value = 0.0;
            switch (group) {
            case 0:
                value = 4000.0 + 3000.0 * wave / 1.2;
                break;
            case 1:
                value = 50.0 + 45.0 * wave / 1.2;
                break;
            case 2:
                value = 0.8 * wave / 1.2;
                break;
            default:
                value = 100.005 + 0.004 * wave / 1.2;
                break;
            }
            dataset.series[s].values[i] = static_cast<float>(value);
        }
    }
    finishDataset(dataset);
    return dataset;
}

Dataset makeSpikesNoisePreset() {
    Dataset dataset = makeBase(60, 36000, 0.1);
    std::mt19937 rng(kSeed ^ 0x51A11CEU);
    std::normal_distribution<float> noise(0.0F, 1.4F);

    for (std::size_t s = 0; s < dataset.series.size(); ++s) {
        const std::size_t period = 503 + (s % 11) * 47;
        const double phase = static_cast<double>(s) * 0.29;
        for (std::size_t i = 0; i < dataset.sampleCount; ++i) {
            const double t = static_cast<double>(i) * dataset.dtSeconds;
            double value = 40.0 + 18.0 * std::sin(0.019 * t + phase)
                + 6.0 * std::sin(0.31 * t + phase * 0.7)
                + static_cast<double>(noise(rng));
            if ((i + s * 13) % period < 2) {
                value += ((i + s) % 2 == 0) ? 55.0 : -45.0;
            }
            dataset.series[s].values[i] = static_cast<float>(value);
        }
    }
    finishDataset(dataset);
    return dataset;
}

Dataset makeStressPreset() {
    constexpr std::size_t seriesCount = 60;
    constexpr std::size_t sampleCount = 360000;
    constexpr double dtSeconds = 0.01;

    Dataset dataset = makeBase(seriesCount, sampleCount, dtSeconds);
    for (std::size_t s = 0; s < seriesCount; ++s) {
        const double phase = static_cast<double>(s) * 0.23;
        const double amplitude = 10.0 + static_cast<double>(s % 9) * 2.0;
        for (std::size_t i = 0; i < sampleCount; ++i) {
            const double t = static_cast<double>(i) * dtSeconds;
            dataset.series[s].values[i] = static_cast<float>(
                50.0 + static_cast<double>(s % 5) * 3.0
                + amplitude * std::sin(0.025 * t + phase)
                + 3.0 * std::sin(0.41 * t + phase * 0.5));
        }
    }
    finishDataset(dataset);
    return dataset;
}
} // namespace

const char* dataPresetName(DataPreset preset) {
    switch (preset) {
    case DataPreset::Small:
        return "Small / Sanity";
    case DataPreset::Reference:
        return "Reference";
    case DataPreset::Overlap:
        return "Overlap";
    case DataPreset::MixedScale:
        return "Mixed Scale";
    case DataPreset::SpikesNoise:
        return "Spikes / Noise";
    case DataPreset::LongTime:
        return "Long Time";
    case DataPreset::StressRaw:
        return "Stress Raw";
    case DataPreset::Count:
    default:
        return "Unknown";
    }
}

Dataset DataGenerator::make(DataPreset preset) {
    switch (preset) {
    case DataPreset::Small:
        return makeSmallPreset();
    case DataPreset::Reference:
        return makeReferencePreset(60, 36000, 0.1);
    case DataPreset::Overlap:
        return makeOverlapPreset();
    case DataPreset::MixedScale:
        return makeMixedScalePreset();
    case DataPreset::SpikesNoise:
        return makeSpikesNoisePreset();
    case DataPreset::LongTime:
        return makeReferencePreset(10, 108000, 0.1);
    case DataPreset::StressRaw:
        return makeStressPreset();
    case DataPreset::Count:
    default:
        return makeSmallPreset();
    }
}

Dataset DataGenerator::makeSmall() {
    return makeSmallPreset();
}
