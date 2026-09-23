#include "MarkerModel.h"

#include "CursorModel.h"

#include <algorithm>

std::size_t MarkerModel::add(double timeSeconds, double datasetEndSeconds) {
    markers_.push_back(TimeMarker{
        .id = nextId_++,
        .timeSeconds = CursorModel::clampTime(timeSeconds, datasetEndSeconds),
    });
    return markers_.size() - 1;
}

void MarkerModel::move(std::size_t index, double timeSeconds, double datasetEndSeconds) {
    if (index >= markers_.size()) {
        return;
    }
    markers_[index].timeSeconds = CursorModel::clampTime(timeSeconds, datasetEndSeconds);
}

void MarkerModel::remove(std::size_t index) {
    if (index >= markers_.size()) {
        return;
    }
    markers_.erase(markers_.begin() + static_cast<std::ptrdiff_t>(index));
}

void MarkerModel::reset() {
    markers_.clear();
    nextId_ = 0;
}
