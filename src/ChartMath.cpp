#include "ChartMath.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace {
constexpr double kZoomBase = 1.15;
constexpr int kMaxTickCount = 2048;

constexpr std::array<double, 29> kTimeSteps{
    0.001, 0.002, 0.005,
    0.010, 0.020, 0.050,
    0.100, 0.200, 0.500,
    1.0, 2.0, 5.0, 10.0, 15.0, 30.0,
    60.0, 120.0, 300.0, 600.0, 900.0, 1800.0,
    3600.0, 7200.0, 10800.0, 21600.0, 43200.0,
    86400.0, 172800.0, 604800.0,
};

double safeMinimumYSpan(double center) {
    return std::max(1.0e-9, std::abs(center) * 1.0e-9);
}

std::vector<double> makeTicksForStep(double minimum, double maximum, double step) {
    std::vector<double> ticks;
    if (!ChartMath::validRange(minimum, maximum) || !(step > 0.0) || !std::isfinite(step)) {
        return ticks;
    }

    const double first = std::ceil(minimum / step) * step;
    if (!std::isfinite(first)) {
        return ticks;
    }

    ticks.reserve(static_cast<std::size_t>(std::min(
        static_cast<double>(kMaxTickCount),
        std::max(0.0, std::floor((maximum - first) / step) + 1.0))));

    for (int i = 0; i < kMaxTickCount; ++i) {
        const double value = first + static_cast<double>(i) * step;
        if (value > maximum + step * 1.0e-9) {
            break;
        }
        if (value >= minimum - step * 1.0e-9) {
            ticks.push_back(std::abs(value) < step * 1.0e-10 ? 0.0 : value);
        }
    }
    return ticks;
}
} // namespace

namespace ChartMath {

bool validRange(double minimum, double maximum) {
    return std::isfinite(minimum) && std::isfinite(maximum) && maximum > minimum;
}

double dataToScreenX(double value, double minimum, double maximum, const PlotRect& rect) {
    if (!validRange(minimum, maximum) || !rect.valid()) {
        return rect.left;
    }
    const double fraction = (value - minimum) / (maximum - minimum);
    return rect.left + fraction * rect.width();
}

double dataToScreenY(double value, double minimum, double maximum, const PlotRect& rect) {
    if (!validRange(minimum, maximum) || !rect.valid()) {
        return rect.bottom;
    }
    const double fraction = (value - minimum) / (maximum - minimum);
    return rect.bottom - fraction * rect.height();
}

double screenToDataX(double x, double minimum, double maximum, const PlotRect& rect) {
    if (!validRange(minimum, maximum) || !rect.valid()) {
        return minimum;
    }
    const double fraction = (x - rect.left) / rect.width();
    return minimum + fraction * (maximum - minimum);
}

double screenToDataY(double y, double minimum, double maximum, const PlotRect& rect) {
    if (!validRange(minimum, maximum) || !rect.valid()) {
        return minimum;
    }
    const double fraction = (rect.bottom - y) / rect.height();
    return minimum + fraction * (maximum - minimum);
}

void panX(double& minimum, double& maximum, double deltaPixels, double plotWidth) {
    if (!validRange(minimum, maximum) || !(plotWidth > 0.0) || !std::isfinite(deltaPixels)) {
        return;
    }
    const double shift = deltaPixels * (maximum - minimum) / plotWidth;
    minimum -= shift;
    maximum -= shift;
}

void zoomX(double& minimum,
           double& maximum,
           double anchorFraction,
           double wheelDelta,
           double minimumSpan) {
    if (!validRange(minimum, maximum) || !std::isfinite(wheelDelta) || wheelDelta == 0.0) {
        return;
    }

    anchorFraction = std::clamp(anchorFraction, 0.0, 1.0);
    const double span = maximum - minimum;
    const double anchor = minimum + anchorFraction * span;
    const double factor = std::pow(kZoomBase, -wheelDelta);
    const double minSpan = std::max(minimumSpan, std::numeric_limits<double>::epsilon());
    const double newSpan = std::max(span * factor, minSpan);

    minimum = anchor - anchorFraction * newSpan;
    maximum = minimum + newSpan;
}

void panY(float& minimum, float& maximum, double deltaPixels, double plotHeight) {
    if (!validRange(minimum, maximum) || !(plotHeight > 0.0) || !std::isfinite(deltaPixels)) {
        return;
    }
    const double shift = deltaPixels * (static_cast<double>(maximum) - minimum) / plotHeight;
    minimum = static_cast<float>(static_cast<double>(minimum) + shift);
    maximum = static_cast<float>(static_cast<double>(maximum) + shift);
}

void zoomY(float& minimum, float& maximum, double wheelDelta) {
    if (!validRange(minimum, maximum) || !std::isfinite(wheelDelta) || wheelDelta == 0.0) {
        return;
    }

    const double minValue = minimum;
    const double maxValue = maximum;
    const double center = 0.5 * (minValue + maxValue);
    const double span = maxValue - minValue;
    const double factor = std::pow(kZoomBase, -wheelDelta);
    const double newSpan = std::max(span * factor, safeMinimumYSpan(center));

    minimum = static_cast<float>(center - 0.5 * newSpan);
    maximum = static_cast<float>(center + 0.5 * newSpan);
}

double niceStep(double rawStep) {
    if (!(rawStep > 0.0) || !std::isfinite(rawStep)) {
        return 1.0;
    }

    const double exponent = std::floor(std::log10(rawStep));
    const double scale = std::pow(10.0, exponent);
    const double fraction = rawStep / scale;

    double niceFraction = 1.0;
    if (fraction <= 1.0) {
        niceFraction = 1.0;
    } else if (fraction <= 2.0) {
        niceFraction = 2.0;
    } else if (fraction <= 5.0) {
        niceFraction = 5.0;
    } else {
        niceFraction = 10.0;
    }
    return niceFraction * scale;
}

std::vector<double> makeTimeTicks(double minimum,
                                  double maximum,
                                  double plotWidth,
                                  double targetSpacingPixels) {
    if (!validRange(minimum, maximum) || !(plotWidth > 0.0) || !(targetSpacingPixels > 0.0)) {
        return {};
    }

    const double desiredCount = std::max(1.0, plotWidth / targetSpacingPixels);
    const double desiredStep = (maximum - minimum) / desiredCount;

    double step = kTimeSteps.back();
    bool found = false;
    for (double candidate : kTimeSteps) {
        if (candidate >= desiredStep) {
            step = candidate;
            found = true;
            break;
        }
    }
    if (!found) {
        step = niceStep(desiredStep);
    }
    return makeTicksForStep(minimum, maximum, step);
}

std::vector<double> makeNiceTicks(double minimum,
                                  double maximum,
                                  double pixelSpan,
                                  double targetSpacingPixels) {
    if (!validRange(minimum, maximum) || !(pixelSpan > 0.0) || !(targetSpacingPixels > 0.0)) {
        return {};
    }

    const double desiredCount = std::max(1.0, pixelSpan / targetSpacingPixels);
    const double step = niceStep((maximum - minimum) / desiredCount);
    return makeTicksForStep(minimum, maximum, step);
}

double pointSegmentDistanceSquared(double px,
                                   double py,
                                   double ax,
                                   double ay,
                                   double bx,
                                   double by) {
    const double dx = bx - ax;
    const double dy = by - ay;
    const double lengthSquared = dx * dx + dy * dy;
    if (!(lengthSquared > 0.0)) {
        const double ex = px - ax;
        const double ey = py - ay;
        return ex * ex + ey * ey;
    }

    const double t = std::clamp(((px - ax) * dx + (py - ay) * dy) / lengthSquared, 0.0, 1.0);
    const double closestX = ax + t * dx;
    const double closestY = ay + t * dy;
    const double ex = px - closestX;
    const double ey = py - closestY;
    return ex * ex + ey * ey;
}

} // namespace ChartMath
