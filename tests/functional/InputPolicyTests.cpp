#include <gtest/gtest.h>

#include "InputPolicy.h"

TEST(InputPolicy, ModifierLeftActionsHavePriority) {
    EXPECT_EQ(InputPolicy::resolveLeftPress(true, false, false, PointerTarget::Marker),
              LeftPressAction::SetCursorA);
    EXPECT_EQ(InputPolicy::resolveLeftPress(false, true, false, PointerTarget::CursorA),
              LeftPressAction::SetCursorB);
    EXPECT_EQ(InputPolicy::resolveLeftPress(false, false, true, PointerTarget::CursorA),
              LeftPressAction::MarkerCandidate);
}

TEST(InputPolicy, PlainLeftUsesOverlayBeforePan) {
    EXPECT_EQ(InputPolicy::resolveLeftPress(false, false, false, PointerTarget::CursorA),
              LeftPressAction::DragCursorA);
    EXPECT_EQ(InputPolicy::resolveLeftPress(false, false, false, PointerTarget::CursorB),
              LeftPressAction::DragCursorB);
    EXPECT_EQ(InputPolicy::resolveLeftPress(false, false, false, PointerTarget::Marker),
              LeftPressAction::DragMarker);
    EXPECT_EQ(InputPolicy::resolveLeftPress(false, false, false, PointerTarget::None),
              LeftPressAction::PanX);
}

TEST(InputPolicy, CtrlRightDoesNotFallThroughToSelection) {
    EXPECT_EQ(InputPolicy::resolveRightPress(false, true, false, PointerTarget::CursorA),
              RightPressAction::HideCursorA);
    EXPECT_EQ(InputPolicy::resolveRightPress(false, true, false, PointerTarget::Marker),
              RightPressAction::RemoveMarker);
    EXPECT_EQ(InputPolicy::resolveRightPress(false, true, false, PointerTarget::None),
              RightPressAction::None);
}

TEST(InputPolicy, PlainRightSelectsSeries) {
    EXPECT_EQ(InputPolicy::resolveRightPress(false, false, false, PointerTarget::None),
              RightPressAction::SelectSeries);
    EXPECT_EQ(InputPolicy::resolveRightPress(false, false, false, PointerTarget::CursorA),
              RightPressAction::SelectSeries);
}

TEST(InputPolicy, WheelModifiersAreExclusive) {
    EXPECT_EQ(InputPolicy::resolveWheel(false, false, false, false), WheelAction::ZoomX);
    EXPECT_EQ(InputPolicy::resolveWheel(false, false, true, true), WheelAction::ZoomY);
    EXPECT_EQ(InputPolicy::resolveWheel(false, false, true, false), WheelAction::None);
    EXPECT_EQ(InputPolicy::resolveWheel(true, false, false, true), WheelAction::None);
    EXPECT_EQ(InputPolicy::resolveWheel(false, true, false, true), WheelAction::None);
}
