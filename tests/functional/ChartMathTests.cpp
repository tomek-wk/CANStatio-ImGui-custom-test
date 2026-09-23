#include <gtest/gtest.h>

#include "ChartMath.h"

#include <cmath>

TEST(ChartMath, DataScreenTransformsRoundTrip) {
    const PlotRect rect{.left = 100.0, .top = 50.0, .right = 900.0, .bottom = 450.0};

    const double x = ChartMath::dataToScreenX(25.0, 0.0, 100.0, rect);
    const double y = ChartMath::dataToScreenY(75.0, 0.0, 100.0, rect);

    EXPECT_DOUBLE_EQ(x, 300.0);
    EXPECT_DOUBLE_EQ(y, 150.0);
    EXPECT_NEAR(ChartMath::screenToDataX(x, 0.0, 100.0, rect), 25.0, 1.0e-9);
    EXPECT_NEAR(ChartMath::screenToDataY(y, 0.0, 100.0, rect), 75.0, 1.0e-9);
}

TEST(ChartMath, PanXUsesGrabContentDirection) {
    double minimum = 0.0;
    double maximum = 100.0;

    ChartMath::panX(minimum, maximum, 100.0, 1000.0);

    EXPECT_NEAR(minimum, -10.0, 1.0e-9);
    EXPECT_NEAR(maximum, 90.0, 1.0e-9);
}

TEST(ChartMath, ZoomXKeepsAnchorFixed) {
    double minimum = 0.0;
    double maximum = 100.0;
    const double anchorFraction = 0.25;
    const double anchorBefore = minimum + anchorFraction * (maximum - minimum);

    ChartMath::zoomX(minimum, maximum, anchorFraction, 1.0, 0.1);

    const double anchorAfter = minimum + anchorFraction * (maximum - minimum);
    EXPECT_NEAR(anchorAfter, anchorBefore, 1.0e-9);
    EXPECT_LT(maximum - minimum, 100.0);
}

TEST(ChartMath, ZoomXRespectsMinimumSpan) {
    double minimum = 0.0;
    double maximum = 1.0;

    ChartMath::zoomX(minimum, maximum, 0.5, 100.0, 0.1);

    EXPECT_GE(maximum - minimum, 0.1 - 1.0e-12);
}

TEST(ChartMath, PanYUsesGrabContentDirection) {
    float minimum = 0.0F;
    float maximum = 100.0F;

    ChartMath::panY(minimum, maximum, 50.0, 500.0);

    EXPECT_NEAR(minimum, 10.0F, 1.0e-5F);
    EXPECT_NEAR(maximum, 110.0F, 1.0e-5F);
}

TEST(ChartMath, ZoomYKeepsCenter) {
    float minimum = 10.0F;
    float maximum = 30.0F;

    ChartMath::zoomY(minimum, maximum, 1.0);

    EXPECT_NEAR(0.5F * (minimum + maximum), 20.0F, 1.0e-5F);
    EXPECT_LT(maximum - minimum, 20.0F);
}

TEST(ChartMath, NiceStepUsesOneTwoFiveFamily) {
    EXPECT_DOUBLE_EQ(ChartMath::niceStep(0.7), 1.0);
    EXPECT_DOUBLE_EQ(ChartMath::niceStep(1.2), 2.0);
    EXPECT_DOUBLE_EQ(ChartMath::niceStep(3.1), 5.0);
    EXPECT_DOUBLE_EQ(ChartMath::niceStep(8.0), 10.0);
}

TEST(ChartMath, TimeTicksStayInsideRange) {
    const auto ticks = ChartMath::makeTimeTicks(3.2, 27.9, 800.0);
    ASSERT_FALSE(ticks.empty());
    for (double tick : ticks) {
        EXPECT_GE(tick, 3.2 - 1.0e-9);
        EXPECT_LE(tick, 27.9 + 1.0e-9);
    }
}

TEST(ChartMath, PointSegmentDistanceUsesSegmentGeometry) {
    EXPECT_NEAR(ChartMath::pointSegmentDistanceSquared(5.0, 3.0, 0.0, 0.0, 10.0, 0.0), 9.0, 1.0e-9);
    EXPECT_NEAR(ChartMath::pointSegmentDistanceSquared(-2.0, 0.0, 0.0, 0.0, 10.0, 0.0), 4.0, 1.0e-9);
}
