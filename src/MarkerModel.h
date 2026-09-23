#pragma once

#include <cstddef>
#include <vector>

struct TimeMarker {
    int id = 0;
    double timeSeconds = 0.0;
};

class MarkerModel {
public:
    [[nodiscard]] const std::vector<TimeMarker>& markers() const { return markers_; }
    [[nodiscard]] std::vector<TimeMarker>& markers() { return markers_; }
    [[nodiscard]] int nextId() const { return nextId_; }

    std::size_t add(double timeSeconds, double datasetEndSeconds);
    void move(std::size_t index, double timeSeconds, double datasetEndSeconds);
    void remove(std::size_t index);
    void reset();

private:
    std::vector<TimeMarker> markers_;
    int nextId_ = 0;
};
