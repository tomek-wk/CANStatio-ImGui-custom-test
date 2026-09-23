#include "CursorModel.h"

#include <gtest/gtest.h>

TEST(CursorModel, StartsHidden) {
    CursorModel cursors;

    EXPECT_FALSE(cursors.a().visible);
    EXPECT_FALSE(cursors.b().visible);
}

TEST(CursorModel, SetsCursorsIndependently) {
    CursorModel cursors;

    cursors.setA(2.5, 10.0);
    cursors.setB(7.5, 10.0);

    EXPECT_TRUE(cursors.a().visible);
    EXPECT_TRUE(cursors.b().visible);
    EXPECT_DOUBLE_EQ(cursors.a().timeSeconds, 2.5);
    EXPECT_DOUBLE_EQ(cursors.b().timeSeconds, 7.5);
}

TEST(CursorModel, ClampsCursorTimeToDataset) {
    CursorModel cursors;

    cursors.setA(-4.0, 10.0);
    cursors.setB(15.0, 10.0);

    EXPECT_DOUBLE_EQ(cursors.a().timeSeconds, 0.0);
    EXPECT_DOUBLE_EQ(cursors.b().timeSeconds, 10.0);
}

TEST(CursorModel, HidesCursorsIndependently) {
    CursorModel cursors;
    cursors.setA(2.0, 10.0);
    cursors.setB(8.0, 10.0);

    cursors.hideA();

    EXPECT_FALSE(cursors.a().visible);
    EXPECT_TRUE(cursors.b().visible);

    cursors.hideB();
    EXPECT_FALSE(cursors.b().visible);
}
