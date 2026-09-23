#include <gtest/gtest.h>

#include "DataGenerator.h"

TEST(DataGenerator, SmallPresetMatchesSpecification) {
    const Dataset dataset = DataGenerator::make(DataPreset::Small);

    EXPECT_EQ(dataset.series.size(), 3U);
    EXPECT_EQ(dataset.sampleCount, 1000U);
    EXPECT_DOUBLE_EQ(dataset.dtSeconds, 0.1);
    EXPECT_EQ(dataset.series[0].name, "Pedal %");
    EXPECT_EQ(dataset.series[1].name, "Throttle %");
    EXPECT_EQ(dataset.series[2].name, "Load %");
}

TEST(DataGenerator, SmallPresetIsDeterministic) {
    const Dataset first = DataGenerator::make(DataPreset::Small);
    const Dataset second = DataGenerator::make(DataPreset::Small);

    ASSERT_EQ(first.series.size(), second.series.size());
    ASSERT_EQ(first.sampleCount, second.sampleCount);
    for (std::size_t s = 0; s < first.series.size(); ++s) {
        ASSERT_EQ(first.series[s].values.size(), second.series[s].values.size());
        EXPECT_EQ(first.series[s].values, second.series[s].values);
    }
}

TEST(DataGenerator, PresetNamesAreStable) {
    EXPECT_STREQ(dataPresetName(DataPreset::Small), "Small / Sanity");
    EXPECT_STREQ(dataPresetName(DataPreset::Reference), "Reference");
    EXPECT_STREQ(dataPresetName(DataPreset::Overlap), "Overlap");
    EXPECT_STREQ(dataPresetName(DataPreset::MixedScale), "Mixed Scale");
    EXPECT_STREQ(dataPresetName(DataPreset::SpikesNoise), "Spikes / Noise");
    EXPECT_STREQ(dataPresetName(DataPreset::LongTime), "Long Time");
    EXPECT_STREQ(dataPresetName(DataPreset::StressRaw), "Stress Raw");
}
