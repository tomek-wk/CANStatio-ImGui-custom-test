#pragma once

#include <vector>

struct PlotRect {
    double left = 0.0;
    double top = 0.0;
    double right = 0.0;
    double bottom = 0.0;

    [[nodiscard]] double width() const { return right - left; }
    [[nodiscard]] double height() const { return bottom - top; }
    [[nodiscard]] bool valid() const { return width() > 0.0 && height() > 0.0; }
};

namespace ChartMath {

[[nodiscard]] bool validRange(double minimum, double maximum);
[[nodiscard]] double dataToScreenX(double value, double minimum, double maximum, const PlotRect& rect);
[[nodiscard]] double dataToScreenY(double value, double minimum, double maximum, const PlotRect& rect);
[[nodiscard]] double screenToDataX(double x, double minimum, double maximum, const PlotRect& rect);
[[nodiscard]] double screenToDataY(double y, double minimum, double maximum, const PlotRect& rect);

void panX(double& minimum, double& maximum, double deltaPixels, double plotWidth);
void zoomX(double& minimum,
           double& maximum,
           double anchorFraction,
           double wheelDelta,
           double minimumSpan);
void panY(float& minimum, float& maximum, double deltaPixels, double plotHeight);
void zoomY(float& minimum, float& maximum, double wheelDelta);

[[nodiscard]] double niceStep(double rawStep);
[[nodiscard]] std::vector<double> makeTimeTicks(double minimum,
                                                double maximum,
                                                double plotWidth,
                                                double targetSpacingPixels = 90.0);
[[nodiscard]] std::vector<double> makeNiceTicks(double minimum,
                                                double maximum,
                                                double pixelSpan,
                                                double targetSpacingPixels = 70.0);

[[nodiscard]] double pointSegmentDistanceSquared(double px,
                                                 double py,
                                                 double ax,
                                                 double ay,
                                                 double bx,
                                                 double by);

} // namespace ChartMath
