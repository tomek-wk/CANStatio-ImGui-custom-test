#include <gtest/gtest.h>

#include "Chart.h"
#include "Dataset.h"
#include "ValuesModel.h"

namespace {
Dataset makeDataset() {
    Dataset dataset;
    dataset.dtSeconds = 1.0;
    dataset.sampleCount = 3;

    Series first;
    first.name = "first";
    first.values = {0.0F, 10.0F, 20.0F};

    Series second;
    second.name = "second";
    second.values = {100.0F, 200.0F, 300.0F};

    Series third;
    third.name = "third";
    third.values = {-10.0F, 0.0F, 10.0F};

    dataset.series = {first, second, third};
    return dataset;
}

ChartMouseState hoveredState(double timeSeconds) {
    ChartMouseState state;
    state.plotHovered = true;
    state.withinDataset = true;
    state.timeSeconds = timeSeconds;
    return state;
}
} // namespace

TEST(ValuesModel, ReadsExactSample) {
    const Dataset dataset = makeDataset();
    const ValuesSnapshot snapshot = makeValuesSnapshot(dataset, hoveredState(1.0));

    ASSERT_EQ(snapshot.rows.size(), 3U);
    EXPECT_DOUBLE_EQ(snapshot.rows[0].value, 10.0);
    EXPECT_DOUBLE_EQ(snapshot.rows[1].value, 200.0);
    EXPECT_DOUBLE_EQ(snapshot.rows[2].value, 0.0);
}

TEST(ValuesModel, InterpolatesHalfwayBetweenSamples) {
    const Dataset dataset = makeDataset();
    const ValuesSnapshot snapshot = makeValuesSnapshot(dataset, hoveredState(0.5));

    ASSERT_EQ(snapshot.rows.size(), 3U);
    EXPECT_DOUBLE_EQ(snapshot.rows[0].value, 5.0);
    EXPECT_DOUBLE_EQ(snapshot.rows[1].value, 150.0);
    EXPECT_DOUBLE_EQ(snapshot.rows[2].value, -5.0);
}

TEST(ValuesModel, InterpolatesQuarterBetweenSamples) {
    const Dataset dataset = makeDataset();
    const ValuesSnapshot snapshot = makeValuesSnapshot(dataset, hoveredState(0.25));

    ASSERT_EQ(snapshot.rows.size(), 3U);
    EXPECT_DOUBLE_EQ(snapshot.rows[0].value, 2.5);
    EXPECT_DOUBLE_EQ(snapshot.rows[1].value, 125.0);
    EXPECT_DOUBLE_EQ(snapshot.rows[2].value, -7.5);
}

TEST(ValuesModel, ReadsDatasetStart) {
    const Dataset dataset = makeDataset();
    const ValuesSnapshot snapshot = makeValuesSnapshot(dataset, hoveredState(0.0));

    ASSERT_EQ(snapshot.rows.size(), 3U);
    EXPECT_DOUBLE_EQ(snapshot.rows[0].value, 0.0);
    EXPECT_DOUBLE_EQ(snapshot.rows[1].value, 100.0);
    EXPECT_DOUBLE_EQ(snapshot.rows[2].value, -10.0);
}

TEST(ValuesModel, ReadsDatasetEnd) {
    const Dataset dataset = makeDataset();
    const ValuesSnapshot snapshot = makeValuesSnapshot(dataset, hoveredState(2.0));

    ASSERT_EQ(snapshot.rows.size(), 3U);
    EXPECT_DOUBLE_EQ(snapshot.rows[0].value, 20.0);
    EXPECT_DOUBLE_EQ(snapshot.rows[1].value, 300.0);
    EXPECT_DOUBLE_EQ(snapshot.rows[2].value, 10.0);
}

TEST(ValuesModel, HiddenSeriesIsOmittedWithoutChangingOrder) {
    Dataset dataset = makeDataset();
    dataset.series[1].visible = false;

    const ValuesSnapshot snapshot = makeValuesSnapshot(dataset, hoveredState(1.0));

    ASSERT_EQ(snapshot.rows.size(), 2U);
    EXPECT_EQ(snapshot.rows[0].seriesIndex, 0U);
    EXPECT_EQ(snapshot.rows[1].seriesIndex, 2U);
}

TEST(ValuesModel, NoHoverReturnsNoRows) {
    const Dataset dataset = makeDataset();
    ChartMouseState state = hoveredState(1.0);
    state.plotHovered = false;

    const ValuesSnapshot snapshot = makeValuesSnapshot(dataset, state);

    EXPECT_FALSE(snapshot.plotHovered);
    EXPECT_TRUE(snapshot.withinDataset);
    EXPECT_TRUE(snapshot.rows.empty());
}

TEST(ValuesModel, OutsideDatasetReturnsNoRows) {
    const Dataset dataset = makeDataset();
    ChartMouseState state = hoveredState(3.0);
    state.withinDataset = false;

    const ValuesSnapshot snapshot = makeValuesSnapshot(dataset, state);

    EXPECT_TRUE(snapshot.plotHovered);
    EXPECT_FALSE(snapshot.withinDataset);
    EXPECT_DOUBLE_EQ(snapshot.timeSeconds, 3.0);
    EXPECT_TRUE(snapshot.rows.empty());
}

TEST(ValuesModel, NonPositiveSampleIntervalReturnsNoRows) {
    Dataset dataset = makeDataset();
    dataset.dtSeconds = 0.0;

    const ValuesSnapshot snapshot = makeValuesSnapshot(dataset, hoveredState(1.0));

    EXPECT_TRUE(snapshot.rows.empty());
}

TEST(ValuesModel, SeriesWithoutValueAtRequestedTimeIsSkipped) {
    Dataset dataset = makeDataset();
    dataset.series[1].values = {100.0F};

    const ValuesSnapshot snapshot = makeValuesSnapshot(dataset, hoveredState(1.0));

    ASSERT_EQ(snapshot.rows.size(), 2U);
    EXPECT_EQ(snapshot.rows[0].seriesIndex, 0U);
    EXPECT_EQ(snapshot.rows[1].seriesIndex, 2U);
}
