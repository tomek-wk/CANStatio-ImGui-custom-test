#include <gtest/gtest.h>

#include "MarkerModel.h"

TEST(MarkerModel, AddsSequentialIdsAndClampsTime) {
    MarkerModel model;

    const std::size_t first = model.add(-5.0, 100.0);
    const std::size_t second = model.add(120.0, 100.0);

    ASSERT_EQ(model.markers().size(), 2U);
    EXPECT_EQ(model.markers()[first].id, 0);
    EXPECT_DOUBLE_EQ(model.markers()[first].timeSeconds, 0.0);
    EXPECT_EQ(model.markers()[second].id, 1);
    EXPECT_DOUBLE_EQ(model.markers()[second].timeSeconds, 100.0);
    EXPECT_EQ(model.nextId(), 2);
}

TEST(MarkerModel, RemovedIdIsNotReused) {
    MarkerModel model;
    model.add(10.0, 100.0);
    model.add(20.0, 100.0);
    model.remove(0);
    model.add(30.0, 100.0);

    ASSERT_EQ(model.markers().size(), 2U);
    EXPECT_EQ(model.markers()[0].id, 1);
    EXPECT_EQ(model.markers()[1].id, 2);
}

TEST(MarkerModel, MoveAndReset) {
    MarkerModel model;
    model.add(10.0, 100.0);
    model.move(0, 75.0, 100.0);
    EXPECT_DOUBLE_EQ(model.markers()[0].timeSeconds, 75.0);

    model.reset();
    EXPECT_TRUE(model.markers().empty());
    EXPECT_EQ(model.nextId(), 0);

    model.add(5.0, 100.0);
    EXPECT_EQ(model.markers()[0].id, 0);
}
