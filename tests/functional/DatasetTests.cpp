#include <gtest/gtest.h>

#include "Dataset.h"

TEST(Dataset, EmptyDatasetEndsAtZero) {
    Dataset dataset;
    dataset.dtSeconds = 0.1;
    dataset.sampleCount = 0;

    EXPECT_DOUBLE_EQ(dataset.endTimeSeconds(), 0.0);
}

TEST(Dataset, EndTimeUsesLastSampleTimestamp) {
    Dataset dataset;
    dataset.dtSeconds = 0.1;
    dataset.sampleCount = 1000;

    EXPECT_DOUBLE_EQ(dataset.endTimeSeconds(), 999.0 * 0.1);
}

TEST(Dataset, SingleSampleEndsAtZero) {
    Dataset dataset;
    dataset.dtSeconds = 0.25;
    dataset.sampleCount = 1;

    EXPECT_DOUBLE_EQ(dataset.endTimeSeconds(), 0.0);
}

TEST(Series, FitViewAddsFivePercentMargin) {
    Series series;
    series.dataMin = 10.0F;
    series.dataMax = 20.0F;

    series.fitViewToData();

    EXPECT_FLOAT_EQ(series.viewMin, 9.5F);
    EXPECT_FLOAT_EQ(series.viewMax, 20.5F);
}

TEST(Series, FitViewHandlesConstantPositiveValue) {
    Series series;
    series.dataMin = 100.0F;
    series.dataMax = 100.0F;

    series.fitViewToData();

    EXPECT_FLOAT_EQ(series.viewMin, 95.0F);
    EXPECT_FLOAT_EQ(series.viewMax, 105.0F);
}

TEST(Series, FitViewHandlesConstantZeroValue) {
    Series series;
    series.dataMin = 0.0F;
    series.dataMax = 0.0F;

    series.fitViewToData();

    EXPECT_FLOAT_EQ(series.viewMin, -0.05F);
    EXPECT_FLOAT_EQ(series.viewMax, 0.05F);
}

TEST(Series, FitViewHandlesConstantNegativeValue) {
    Series series;
    series.dataMin = -20.0F;
    series.dataMax = -20.0F;

    series.fitViewToData();

    EXPECT_FLOAT_EQ(series.viewMin, -21.0F);
    EXPECT_FLOAT_EQ(series.viewMax, -19.0F);
}
