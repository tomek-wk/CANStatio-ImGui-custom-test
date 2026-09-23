#include "CursorModel.h"

#include <algorithm>

void CursorModel::setA(double timeSeconds, double datasetEndSeconds) {
    a_.visible = true;
    a_.timeSeconds = clampTime(timeSeconds, datasetEndSeconds);
}

void CursorModel::setB(double timeSeconds, double datasetEndSeconds) {
    b_.visible = true;
    b_.timeSeconds = clampTime(timeSeconds, datasetEndSeconds);
}

void CursorModel::hideA() {
    a_.visible = false;
}

void CursorModel::hideB() {
    b_.visible = false;
}

void CursorModel::reset() {
    a_ = {};
    b_ = {};
}

double CursorModel::clampTime(double timeSeconds, double datasetEndSeconds) {
    return std::clamp(timeSeconds, 0.0, std::max(0.0, datasetEndSeconds));
}
