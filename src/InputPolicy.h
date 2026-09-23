#pragma once

enum class PointerTarget {
    None,
    CursorA,
    CursorB,
    Marker,
};

enum class LeftPressAction {
    SetCursorA,
    SetCursorB,
    MarkerCandidate,
    DragCursorA,
    DragCursorB,
    DragMarker,
    PanX,
};

enum class RightPressAction {
    None,
    HideCursorA,
    HideCursorB,
    RemoveMarker,
    SelectSeries,
};

enum class WheelAction {
    None,
    ZoomX,
    ZoomY,
};

namespace InputPolicy {

inline LeftPressAction resolveLeftPress(bool shift,
                                        bool ctrl,
                                        bool alt,
                                        PointerTarget target) {
    if (shift) {
        return LeftPressAction::SetCursorA;
    }
    if (ctrl) {
        return LeftPressAction::SetCursorB;
    }
    if (alt) {
        return LeftPressAction::MarkerCandidate;
    }

    switch (target) {
    case PointerTarget::CursorA:
        return LeftPressAction::DragCursorA;
    case PointerTarget::CursorB:
        return LeftPressAction::DragCursorB;
    case PointerTarget::Marker:
        return LeftPressAction::DragMarker;
    case PointerTarget::None:
    default:
        return LeftPressAction::PanX;
    }
}

inline RightPressAction resolveRightPress(bool shift,
                                          bool ctrl,
                                          bool alt,
                                          PointerTarget target) {
    if (ctrl) {
        switch (target) {
        case PointerTarget::CursorA:
            return RightPressAction::HideCursorA;
        case PointerTarget::CursorB:
            return RightPressAction::HideCursorB;
        case PointerTarget::Marker:
            return RightPressAction::RemoveMarker;
        case PointerTarget::None:
        default:
            return RightPressAction::None;
        }
    }
    if (shift || alt) {
        return RightPressAction::None;
    }
    return RightPressAction::SelectSeries;
}

inline WheelAction resolveWheel(bool shift,
                                bool ctrl,
                                bool alt,
                                bool hasActiveVisibleSeries) {
    if (alt) {
        return hasActiveVisibleSeries ? WheelAction::ZoomY : WheelAction::None;
    }
    if (shift || ctrl) {
        return WheelAction::None;
    }
    return WheelAction::ZoomX;
}

} // namespace InputPolicy
