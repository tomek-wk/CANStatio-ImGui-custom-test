#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

struct Series {
    std::string name;
    std::vector<float> values;
    float dataMin = 0.0F;
    float dataMax = 0.0F;
    float viewMin = 0.0F;
    float viewMax = 1.0F;
    bool visible = true;

    void fitViewToData() {
        const float span = dataMax - dataMin;
        if (span > 0.0F) {
            const float margin = span * 0.05F;
            viewMin = dataMin - margin;
            viewMax = dataMax + margin;
            return;
        }

        const float scale = std::max(std::abs(dataMin), 1.0F);
        const float halfRange = scale * 0.05F;
        viewMin = dataMin - halfRange;
        viewMax = dataMax + halfRange;
    }
};

struct Dataset {
    double dtSeconds = 0.1;
    std::size_t sampleCount = 0;
    std::vector<Series> series;

    [[nodiscard]] double endTimeSeconds() const {
        if (sampleCount == 0) {
            return 0.0;
        }
        return static_cast<double>(sampleCount - 1) * dtSeconds;
    }
};
