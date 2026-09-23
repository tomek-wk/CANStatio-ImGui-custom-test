#include "ValuesModel.h"

#include "Chart.h"
#include "Dataset.h"

#include <algorithm>
#include <cmath>

namespace {
bool interpolateValue(const Series& series,
                      double dtSeconds,
                      double timeSeconds,
                      double& value) {
    if (series.values.empty() || !(dtSeconds > 0.0) || timeSeconds < 0.0) {
        return false;
    }

    const double lastTime = static_cast<double>(series.values.size() - 1) * dtSeconds;
    if (timeSeconds > lastTime) {
        return false;
    }

    const double samplePosition = timeSeconds / dtSeconds;
    const std::size_t lowerIndex = std::min(
        static_cast<std::size_t>(std::floor(samplePosition)),
        series.values.size() - 1);
    const std::size_t upperIndex = std::min(lowerIndex + 1, series.values.size() - 1);

    if (lowerIndex == upperIndex) {
        value = static_cast<double>(series.values[lowerIndex]);
        return true;
    }

    const double fraction = std::clamp(samplePosition - static_cast<double>(lowerIndex), 0.0, 1.0);
    const double lower = static_cast<double>(series.values[lowerIndex]);
    const double upper = static_cast<double>(series.values[upperIndex]);
    value = lower + (upper - lower) * fraction;
    return true;
}
} // namespace

ValuesSnapshot makeValuesSnapshot(const Dataset& dataset,
                                  const ChartMouseState& mouseState) {
    ValuesSnapshot snapshot;
    snapshot.plotHovered = mouseState.plotHovered;
    snapshot.withinDataset = mouseState.withinDataset;
    snapshot.timeSeconds = mouseState.timeSeconds;

    if (!snapshot.plotHovered || !snapshot.withinDataset) {
        return snapshot;
    }

    snapshot.rows.reserve(dataset.series.size());
    for (std::size_t i = 0; i < dataset.series.size(); ++i) {
        const Series& series = dataset.series[i];
        if (!series.visible) {
            continue;
        }

        double value = 0.0;
        if (!interpolateValue(series, dataset.dtSeconds, snapshot.timeSeconds, value)) {
            continue;
        }

        snapshot.rows.push_back(ValuesRow{.seriesIndex = i, .value = value});
    }

    return snapshot;
}
