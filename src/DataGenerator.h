#pragma once

#include "Dataset.h"

enum class DataPreset {
    Small,
    Reference,
    Overlap,
    MixedScale,
    SpikesNoise,
    LongTime,
    StressRaw,
    Count,
};

[[nodiscard]] const char* dataPresetName(DataPreset preset);

class DataGenerator {
public:
    [[nodiscard]] static Dataset make(DataPreset preset);
    [[nodiscard]] static Dataset makeSmall();
};
